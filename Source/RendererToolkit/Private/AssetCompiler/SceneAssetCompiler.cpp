/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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

#include <Renderer/Public/Asset/AssetPackage.h>
#include <Renderer/Public/Core/Math/Math.h>
#include <Renderer/Public/Core/File/MemoryFile.h>
#include <Renderer/Public/Core/File/FileSystemHelper.h>
#include <Renderer/Public/Resource/Scene/Item/Camera/CameraSceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Debug/DebugDrawSceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Grass/GrassSceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Light/SunlightSceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Particles/ParticlesSceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Sky/SkySceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Terrain/TerrainSceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Volume/VolumeSceneItem.h>
#include <Renderer/Public/Resource/Scene/Loader/SceneFileFormat.h>
#include <Renderer/Public/Resource/Material/MaterialProperties.h>
#include <Renderer/Public/Resource/Material/MaterialResourceManager.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4297)	// warning C4297: 'rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>::Clear': function assumed not to throw an exception but does
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
		//[ Functions                                             ]
		//[-------------------------------------------------------]
		void optionalLightTypeProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, Renderer::LightSceneItem::LightType& value)
		{
			if (rapidJsonValue.HasMember(propertyName))
			{
				const rapidjson::Value& rapidJsonValueValueType = rapidJsonValue[propertyName];
				const char* valueAsString = rapidJsonValueValueType.GetString();
				const rapidjson::SizeType valueStringLength = rapidJsonValueValueType.GetStringLength();

				// Define helper macros
				#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Renderer::LightSceneItem::LightType::name;
				#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) value = Renderer::LightSceneItem::LightType::name;

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

		void fillSortedMaterialPropertyVector(const RendererToolkit::IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueSceneItem, Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector)
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
					typedef std::unordered_map<uint32_t, std::string> DefinedMaterialPropertyIds;	// Key = "Renderer::Renderer::MaterialPropertyId"
					DefinedMaterialPropertyIds definedMaterialPropertyIds;
					for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorProperties = rapidJsonValueProperties.MemberBegin(); rapidJsonMemberIteratorProperties != rapidJsonValueProperties.MemberEnd(); ++rapidJsonMemberIteratorProperties)
					{
						definedMaterialPropertyIds.emplace(Renderer::MaterialPropertyId(rapidJsonMemberIteratorProperties->name.GetString()), rapidJsonMemberIteratorProperties->value.GetString());
					}

					// Mark material properties as overwritten
					for (Renderer::MaterialProperty& materialProperty : sortedMaterialPropertyVector)
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

		void readMaterialSceneData(const RendererToolkit::IAssetCompiler::Input& input, const Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, const rapidjson::Value& rapidJsonValueSceneItem, Renderer::v1Scene::MaterialData& materialData)
		{
			// Set data
			Renderer::AssetId materialAssetId;
			Renderer::AssetId materialBlueprintAssetId;
			RendererToolkit::JsonHelper::optionalCompiledAssetId(input, rapidJsonValueSceneItem, "Material", materialAssetId);
			RendererToolkit::JsonHelper::optionalStringIdProperty(rapidJsonValueSceneItem, "MaterialTechnique", materialData.materialTechniqueId);
			RendererToolkit::JsonHelper::optionalCompiledAssetId(input, rapidJsonValueSceneItem, "MaterialBlueprint", materialBlueprintAssetId);
			materialData.materialAssetId = materialAssetId;
			materialData.materialBlueprintAssetId = materialBlueprintAssetId;
			materialData.numberOfMaterialProperties = static_cast<uint32_t>(sortedMaterialPropertyVector.size());

			// Sanity checks
			if (Renderer::isInvalid(materialData.materialAssetId) && Renderer::isInvalid(materialData.materialBlueprintAssetId))
			{
				throw std::runtime_error("Material asset ID or material blueprint asset ID must be defined");
			}
			if (Renderer::isValid(materialData.materialAssetId) && Renderer::isValid(materialData.materialBlueprintAssetId))
			{
				throw std::runtime_error("Material asset ID is defined, but material blueprint asset ID is defined as well. Only one asset ID is allowed.");
			}
			if (Renderer::isInvalid(materialData.materialTechniqueId))
			{
				materialData.materialTechniqueId = Renderer::MaterialResourceManager::DEFAULT_MATERIAL_TECHNIQUE_ID;
			}
		}

		void writeItemHeader(Renderer::IFile& file, Renderer::SceneItemTypeId typeId, uint32_t numberOfBytes)
		{
			Renderer::v1Scene::ItemHeader itemHeader;
			itemHeader.typeId		 = typeId;
			itemHeader.numberOfBytes = numberOfBytes;
			file.write(&itemHeader, sizeof(Renderer::v1Scene::ItemHeader));
		}

		void writeMaterialData(Renderer::IFile& file, const RendererToolkit::IAssetCompiler::Input& input, const rapidjson::Value& rapidJsonValueItem, const Renderer::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector)
		{
			// Gather data
			Renderer::v1Scene::MaterialData materialData;
			::detail::readMaterialSceneData(input, sortedMaterialPropertyVector, rapidJsonValueItem, materialData);

			// Write down
			file.write(&materialData, sizeof(Renderer::v1Scene::MaterialData));
			if (!sortedMaterialPropertyVector.empty())
			{
				// Write down all material properties
				file.write(sortedMaterialPropertyVector.data(), sizeof(Renderer::MaterialProperty) * sortedMaterialPropertyVector.size());
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
		return (input.virtualAssetOutputDirectory + '/' + std_filesystem::path(input.virtualAssetFilename).stem().generic_string()).append(getOptionalUniqueAssetFilenameExtension());
	}

	bool SceneAssetCompiler::checkIfChanged(const Input& input, const Configuration& configuration) const
	{
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + JsonHelper::getAssetInputFileByRapidJsonDocument(configuration.rapidJsonDocumentAsset);
		return input.cacheManager.checkIfFileIsModified(configuration.rhiTarget, input.virtualAssetFilename, {virtualInputFilename}, getVirtualOutputAssetFilename(input, configuration), Renderer::v1Scene::FORMAT_VERSION);
	}

	void SceneAssetCompiler::compile(const Input& input, const Configuration& configuration) const
	{
		// Get relevant data
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + JsonHelper::getAssetInputFileByRapidJsonDocument(configuration.rapidJsonDocumentAsset);
		const std::string virtualOutputAssetFilename = getVirtualOutputAssetFilename(input, configuration);

		// Ask the cache manager whether or not we need to compile the source file (e.g. source changed or target not there)
		CacheManager::CacheEntries cacheEntries;
		if (input.cacheManager.needsToBeCompiled(configuration.rhiTarget, input.virtualAssetFilename, virtualInputFilename, virtualOutputAssetFilename, Renderer::v1Scene::FORMAT_VERSION, cacheEntries))
		{
			Renderer::MemoryFile memoryFile(0, 4096);

			{ // Scene
				// Parse JSON
				rapidjson::Document rapidJsonDocument;
				JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualInputFilename, "SceneAsset", "1", rapidJsonDocument);

				{ // Write down the scene resource header
					Renderer::v1Scene::SceneHeader sceneHeader;
					sceneHeader.unused = 42;	// TODO(co) Currently the scene header is unused
					memoryFile.write(&sceneHeader, sizeof(Renderer::v1Scene::SceneHeader));
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
					Renderer::v1Scene::Nodes nodes;
					nodes.numberOfNodes = numberOfNodes;
					memoryFile.write(&nodes, sizeof(Renderer::v1Scene::Nodes));

					// Loop through all scene nodes
					for (rapidjson::SizeType nodeIndex = 0; nodeIndex < numberOfNodes; ++nodeIndex)
					{
						const rapidjson::Value& rapidJsonValueNode = rapidJsonValueNodes[nodeIndex];
						const rapidjson::Value* rapidJsonValueItems = rapidJsonValueNode.HasMember("Items") ? &rapidJsonValueNode["Items"] : nullptr;

						{ // Write down the scene node
							Renderer::v1Scene::Node node;

							// Get the scene node transform
							node.transform.scale = Renderer::Math::VEC3_ONE;
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
							memoryFile.write(&node, sizeof(Renderer::v1Scene::Node));
						}

						// Write down the scene items
						if (nullptr != rapidJsonValueItems)
						{
							for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorItems = rapidJsonValueItems->MemberBegin(); rapidJsonMemberIteratorItems != rapidJsonValueItems->MemberEnd(); ++rapidJsonMemberIteratorItems)
							{
								const rapidjson::Value& rapidJsonValueItem = rapidJsonMemberIteratorItems->value;
								const Renderer::SceneItemTypeId typeId = Renderer::StringId(rapidJsonMemberIteratorItems->name.GetString());

								// TODO(co) Make this more generic via scene factory
								switch (typeId)
								{
									case Renderer::CameraSceneItem::TYPE_ID:
									{
										// Write down the scene item header
										::detail::writeItemHeader(memoryFile, typeId, 0);
										break;
									}

									case Renderer::LightSceneItem::TYPE_ID:
									{
										// Write down the scene item header
										::detail::writeItemHeader(memoryFile, typeId, sizeof(Renderer::v1Scene::LightItem));

										// Read properties
										Renderer::v1Scene::LightItem lightItem;
										::detail::optionalLightTypeProperty(rapidJsonValueItem, "LightType", lightItem.lightType);
										JsonHelper::optionalRgbColorProperty(rapidJsonValueItem, "Color", lightItem.color);
										JsonHelper::optionalUnitNProperty(rapidJsonValueItem, "Radius", &lightItem.radius, 1);
										JsonHelper::optionalAngleProperty(rapidJsonValueItem, "InnerAngle", lightItem.innerAngle);
										JsonHelper::optionalAngleProperty(rapidJsonValueItem, "OuterAngle", lightItem.outerAngle);
										JsonHelper::optionalUnitNProperty(rapidJsonValueItem, "NearClipDistance", &lightItem.nearClipDistance, 1);
										JsonHelper::optionalIntegerNProperty(rapidJsonValueItem, "IesLightProfileIndex", &lightItem.iesLightProfileIndex, 1);

										// Sanity checks
										if (lightItem.color[0] < 0.0f || lightItem.color[1] < 0.0f || lightItem.color[2] < 0.0f)
										{
											throw std::runtime_error("All light item color components must be positive");
										}
										if (Renderer::LightSceneItem::LightType::DIRECTIONAL != lightItem.lightType && lightItem.radius <= 0.0f)
										{
											throw std::runtime_error("For point or spot light items the radius must be greater as zero");
										}
										if (Renderer::LightSceneItem::LightType::DIRECTIONAL == lightItem.lightType && 0.0f != lightItem.radius)
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
										if (lightItem.iesLightProfileIndex >= 0 && (rapidJsonValueItem.HasMember("InnerAngle") || rapidJsonValueItem.HasMember("OuterAngle")))
										{
											throw std::runtime_error("\"InnerAngle\" and \"OuterAngle\" are unused if \"IesLightProfileIndex\" is used");
										}

										// Write down
										memoryFile.write(&lightItem, sizeof(Renderer::v1Scene::LightItem));
										break;
									}

									case Renderer::SunlightSceneItem::TYPE_ID:
									{
										// Write down the scene item header
										::detail::writeItemHeader(memoryFile, typeId, sizeof(Renderer::v1Scene::SunlightItem));

										// Read properties
										Renderer::v1Scene::SunlightItem sunlightItem;
										JsonHelper::optionalTimeOfDayProperty(rapidJsonValueItem, "SunriseTime", sunlightItem.sunriseTime);
										JsonHelper::optionalTimeOfDayProperty(rapidJsonValueItem, "SunsetTime", sunlightItem.sunsetTime);
										JsonHelper::optionalAngleProperty(rapidJsonValueItem, "EastDirection", sunlightItem.eastDirection);
										JsonHelper::optionalAngleProperty(rapidJsonValueItem, "AngleOfIncidence", sunlightItem.angleOfIncidence);
										JsonHelper::optionalTimeOfDayProperty(rapidJsonValueItem, "TimeOfDay", sunlightItem.timeOfDay);

										// Write down
										memoryFile.write(&sunlightItem, sizeof(Renderer::v1Scene::SunlightItem));
										break;
									}

									case Renderer::MeshSceneItem::TYPE_ID:
									case Renderer::SkeletonMeshSceneItem::TYPE_ID:
									{
										// Get the number of scene item bytes
										uint32_t numberOfBytes = sizeof(Renderer::v1Scene::MeshItem) + sizeof(Renderer::AssetId) * (rapidJsonValueItem.HasMember("SubMeshMaterials") ? rapidJsonValueItem["SubMeshMaterials"].Size() : 0);
										if (Renderer::SkeletonMeshSceneItem::TYPE_ID == typeId)
										{
											numberOfBytes += sizeof(Renderer::v1Scene::SkeletonMeshItem);
										}

										// Write down the scene item header
										::detail::writeItemHeader(memoryFile, typeId, numberOfBytes);

										// Skeleton mesh scene item
										if (Renderer::SkeletonMeshSceneItem::TYPE_ID == typeId)
										{
											Renderer::v1Scene::SkeletonMeshItem skeletonMeshItem;

											// Optional skeleton animation: Map the source asset ID to the compiled asset ID
											skeletonMeshItem.skeletonAnimationAssetId = Renderer::getInvalid<Renderer::AssetId>();
											JsonHelper::optionalCompiledAssetId(input, rapidJsonValueItem, "SkeletonAnimation", skeletonMeshItem.skeletonAnimationAssetId);

											// Write down
											memoryFile.write(&skeletonMeshItem, sizeof(Renderer::v1Scene::SkeletonMeshItem));
										}

										// Mesh scene item
										Renderer::v1Scene::MeshItem meshItem;

										// Map the source asset ID to the compiled asset ID
										meshItem.meshAssetId = JsonHelper::getCompiledAssetId(input, rapidJsonValueItem, "Mesh");

										// Optional sub-mesh material asset IDs to be able to overwrite the original material asset ID of sub-meshes
										std::vector<Renderer::AssetId> subMeshMaterialAssetIds;
										if (rapidJsonValueItem.HasMember("SubMeshMaterials"))
										{
											const rapidjson::Value& rapidJsonValueSubMeshMaterialAssetIds = rapidJsonValueItem["SubMeshMaterials"];
											const uint32_t numberOfSubMeshMaterialAssetIds = rapidJsonValueSubMeshMaterialAssetIds.Size();
											subMeshMaterialAssetIds.resize(numberOfSubMeshMaterialAssetIds);
											for (uint32_t i = 0; i < numberOfSubMeshMaterialAssetIds; ++i)
											{
												// Empty string means "Don't overwrite the original material asset ID of the sub-mesh"
												const std::string valueAsString = rapidJsonValueSubMeshMaterialAssetIds[i].GetString();
												subMeshMaterialAssetIds[i] = valueAsString.empty() ? Renderer::getInvalid<Renderer::AssetId>() : StringHelper::getAssetIdByString(valueAsString, input);
											}
										}
										meshItem.numberOfSubMeshMaterialAssetIds = static_cast<uint32_t>(subMeshMaterialAssetIds.size());

										// Write down
										memoryFile.write(&meshItem, sizeof(Renderer::v1Scene::MeshItem));
										if (!subMeshMaterialAssetIds.empty())
										{
											// Write down all sub-mesh material asset IDs
											memoryFile.write(subMeshMaterialAssetIds.data(), sizeof(Renderer::AssetId) * subMeshMaterialAssetIds.size());
										}
										break;
									}

									case Renderer::DebugDrawSceneItem::TYPE_ID:
									{
										static constexpr char TYPE_NAME[Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES][24] = { "PointListDepthDisabled", "PointListDepthEnabled", "LineListDepthDisabled", "LineListDepthEnabled", "GlyphList" };

										// Get mandatory JSON values of the types
										rapidjson::Value* rapidJsonValuePerType[Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES];
										for (uint32_t i = 0; i < Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES; ++i)
										{
											rapidJsonValuePerType[i] = const_cast<rapidjson::Value*>(&rapidJsonValueItem[TYPE_NAME[i]]);	// Evil const-cast
										}

										// Get material properties
										Renderer::MaterialProperties::SortedPropertyVector sortedMaterialPropertyVectors[Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES];
										for (uint32_t i = 0; i < Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES; ++i)
										{
											::detail::fillSortedMaterialPropertyVector(input, *rapidJsonValuePerType[i], sortedMaterialPropertyVectors[i]);
										}

										{ // Write down the scene item header
											uint32_t numberOfBytes = static_cast<uint32_t>(sizeof(Renderer::v1Scene::MaterialData)) * Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES;
											for (uint32_t i = 0; i < Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES; ++i)
											{
												numberOfBytes += static_cast<uint32_t>(sizeof(Renderer::MaterialProperty) * sortedMaterialPropertyVectors[i].size());
											}
											::detail::writeItemHeader(memoryFile, typeId, numberOfBytes);
										}

										// Write material data
										for (uint32_t i = 0; i < Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES; ++i)
										{
											detail::writeMaterialData(memoryFile, input, *rapidJsonValuePerType[i], sortedMaterialPropertyVectors[i]);
										}
										break;
									}

									case Renderer::SkySceneItem::TYPE_ID:
									case Renderer::VolumeSceneItem::TYPE_ID:
									case Renderer::GrassSceneItem::TYPE_ID:
									case Renderer::TerrainSceneItem::TYPE_ID:
									case Renderer::ParticlesSceneItem::TYPE_ID:
									{
										// Get material properties
										Renderer::MaterialProperties::SortedPropertyVector sortedMaterialPropertyVector;
										::detail::fillSortedMaterialPropertyVector(input, rapidJsonValueItem, sortedMaterialPropertyVector);

										// Write down the scene item header
										::detail::writeItemHeader(memoryFile, typeId, static_cast<uint32_t>(sizeof(Renderer::v1Scene::MaterialData) + sizeof(Renderer::MaterialProperty) * sortedMaterialPropertyVector.size()));

										// Write material data
										detail::writeMaterialData(memoryFile, input, rapidJsonValueItem, sortedMaterialPropertyVector);
										break;
									}

									default:
									{
										// Error!
										throw std::runtime_error("Scene item type \"" + std::string(rapidJsonMemberIteratorItems->name.GetString()) + "\" is unknown");
									}
								}
							}
						}
					}
				}
			}

			// Write LZ4 compressed output
			if (!memoryFile.writeLz4CompressedDataByVirtualFilename(Renderer::v1Scene::FORMAT_TYPE, Renderer::v1Scene::FORMAT_VERSION, input.context.getFileManager(), virtualOutputAssetFilename.c_str()))
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
