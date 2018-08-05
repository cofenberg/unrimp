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
#include "RendererToolkit/AssetCompiler/CompositorNodeAssetCompiler.h"
#include "RendererToolkit/Helper/JsonMaterialBlueprintHelper.h"
#include "RendererToolkit/Helper/JsonMaterialHelper.h"
#include "RendererToolkit/Helper/CacheManager.h"
#include "RendererToolkit/Helper/StringHelper.h"
#include "RendererToolkit/Helper/JsonHelper.h"
#include "RendererToolkit/Context.h"

#include <RendererRuntime/Asset/AssetPackage.h>
#include <RendererRuntime/Core/File/MemoryFile.h>
#include <RendererRuntime/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Resource/Material/MaterialResourceManager.h>
#include <RendererRuntime/Resource/CompositorNode/Loader/CompositorNodeFileFormat.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/Copy/CompositorResourcePassCopy.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/Compute/CompositorResourcePassCompute.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/GenerateMipmaps/CompositorResourcePassGenerateMipmaps.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/Clear/CompositorResourcePassClear.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/DebugGui/CompositorResourcePassDebugGui.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/ShadowMap/CompositorResourcePassShadowMap.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/VrHiddenAreaMesh/CompositorResourcePassVrHiddenAreaMesh.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/ResolveMultisample/CompositorResourcePassResolveMultisample.h>

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

#include <unordered_set>


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
		void fillSortedMaterialPropertyVector(const RendererToolkit::IAssetCompiler::Input& input, const std::unordered_set<uint32_t>& renderTargetTextureAssetIds, const rapidjson::Value& rapidJsonValuePass, RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector)
		{
			// Check whether or not material properties should be set
			if (rapidJsonValuePass.HasMember("SetMaterialProperties"))
			{
				if (rapidJsonValuePass.HasMember("Material"))
				{
					RendererToolkit::JsonMaterialHelper::getPropertiesByMaterialAssetId(input, RendererToolkit::StringHelper::getSourceAssetIdByString(rapidJsonValuePass["Material"].GetString(), input), sortedMaterialPropertyVector);
				}
				else if (rapidJsonValuePass.HasMember("MaterialBlueprint"))
				{
					RendererToolkit::JsonMaterialBlueprintHelper::getPropertiesByMaterialBlueprintAssetId(input, RendererToolkit::StringHelper::getSourceAssetIdByString(rapidJsonValuePass["MaterialBlueprint"].GetString(), input), sortedMaterialPropertyVector);
				}
				if (!sortedMaterialPropertyVector.empty())
				{
					// Update material property values were required
					const rapidjson::Value& rapidJsonValueProperties = rapidJsonValuePass["SetMaterialProperties"];
					RendererToolkit::JsonMaterialHelper::readMaterialPropertyValues(input, rapidJsonValueProperties, sortedMaterialPropertyVector);

					{ // Need a second round for referenced render target textures so we can write e.g. "ColorMap": "ColorRenderTargetTexture0" ("ColorRenderTargetTexture0" = render target texture)
						// Collect all material property IDs explicitly defined inside the compositor node asset
						typedef std::unordered_map<uint32_t, std::string> DefinedMaterialPropertyIds;	// Key = "RendererRuntime::RendererRuntime::MaterialPropertyId"
						DefinedMaterialPropertyIds definedMaterialPropertyIds;
						for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorProperties = rapidJsonValueProperties.MemberBegin(); rapidJsonMemberIteratorProperties != rapidJsonValueProperties.MemberEnd(); ++rapidJsonMemberIteratorProperties)
						{
							definedMaterialPropertyIds.emplace(RendererRuntime::MaterialPropertyId(rapidJsonMemberIteratorProperties->name.GetString()), rapidJsonMemberIteratorProperties->value.GetString());
						}

						// Mark material properties as overwritten and update texture asset IDs if necessary
						for (RendererRuntime::MaterialProperty& materialProperty : sortedMaterialPropertyVector)
						{
							DefinedMaterialPropertyIds::const_iterator iterator = definedMaterialPropertyIds.find(materialProperty.getMaterialPropertyId());
							if (iterator!= definedMaterialPropertyIds.end())
							{
								materialProperty.setOverwritten(true);
								if (materialProperty.getValueType() == RendererRuntime::MaterialPropertyValue::ValueType::TEXTURE_ASSET_ID)
								{
									const RendererRuntime::AssetId assetId = RendererToolkit::StringHelper::getAssetIdByString(iterator->second);
									if (renderTargetTextureAssetIds.find(assetId) != renderTargetTextureAssetIds.end())
									{
										static_cast<RendererRuntime::MaterialPropertyValue&>(materialProperty) = RendererRuntime::MaterialPropertyValue::fromTextureAssetId(assetId);
									}
								}
							}
						}
					}
				}
			}
		}

		void readPass(const rapidjson::Value& rapidJsonValuePass, RendererRuntime::v1CompositorNode::Pass& pass)
		{
			// Read properties
			RendererToolkit::JsonHelper::optionalFloatProperty(rapidJsonValuePass, "MinimumDepth", pass.minimumDepth);
			RendererToolkit::JsonHelper::optionalFloatProperty(rapidJsonValuePass, "MaximumDepth", pass.maximumDepth);
			RendererToolkit::JsonHelper::optionalIntegerProperty(rapidJsonValuePass, "NumberOfExecutions", pass.numberOfExecutions);
			RendererToolkit::JsonHelper::optionalBooleanProperty(rapidJsonValuePass, "SkipFirstExecution", pass.skipFirstExecution);

			// Sanity checks
			if (0 == pass.numberOfExecutions)
			{
				throw std::runtime_error("The number of compositor pass executions can't be zero");
			}
			if (pass.skipFirstExecution && 1 == pass.numberOfExecutions)
			{
				throw std::runtime_error("The first execution of the compositor pass is skipped, but the number of compositor pass executions is set to one resulting in that the compositor pass will never be executed");
			}
		}

		void readPassCompute(const RendererToolkit::IAssetCompiler::Input& input, const RendererRuntime::MaterialProperties::SortedPropertyVector& sortedMaterialPropertyVector, const rapidjson::Value& rapidJsonValuePass, bool materialDefinitionMandatory, RendererRuntime::v1CompositorNode::PassCompute& passCompute)
		{
			// Set data
			RendererRuntime::AssetId materialAssetId;
			RendererRuntime::AssetId materialBlueprintAssetId;
			RendererToolkit::JsonHelper::optionalCompiledAssetId(input, rapidJsonValuePass, "Material", materialAssetId);
			RendererToolkit::JsonHelper::optionalStringIdProperty(rapidJsonValuePass, "MaterialTechnique", passCompute.materialTechniqueId);
			RendererToolkit::JsonHelper::optionalCompiledAssetId(input, rapidJsonValuePass, "MaterialBlueprint", materialBlueprintAssetId);
			passCompute.materialAssetId = materialAssetId;
			passCompute.materialBlueprintAssetId = materialBlueprintAssetId;
			passCompute.numberOfMaterialProperties = static_cast<uint32_t>(sortedMaterialPropertyVector.size());

			// Sanity checks
			if (materialDefinitionMandatory && RendererRuntime::isInvalid(passCompute.materialAssetId) && RendererRuntime::isInvalid(passCompute.materialBlueprintAssetId))
			{
				throw std::runtime_error("Material asset ID or material blueprint asset ID must be defined");
			}
			if (RendererRuntime::isValid(passCompute.materialAssetId) && RendererRuntime::isValid(passCompute.materialBlueprintAssetId))
			{
				throw std::runtime_error("Material asset ID is defined, but material blueprint asset ID is defined as well. Only one asset ID is allowed.");
			}
			if (RendererRuntime::isValid(passCompute.materialAssetId) && RendererRuntime::isInvalid(passCompute.materialTechniqueId))
			{
				throw std::runtime_error("Material asset ID is defined, but material technique is not defined");
			}
			if (RendererRuntime::isValid(passCompute.materialBlueprintAssetId) && RendererRuntime::isInvalid(passCompute.materialTechniqueId))
			{
				passCompute.materialTechniqueId = RendererRuntime::MaterialResourceManager::DEFAULT_MATERIAL_TECHNIQUE_ID;
			}
		}

		void readPassScene(const rapidjson::Value& rapidJsonValuePass, RendererRuntime::v1CompositorNode::PassScene& passScene)
		{
			// Read properties
			RendererToolkit::JsonHelper::optionalByteProperty(rapidJsonValuePass, "MinimumRenderQueueIndex", passScene.minimumRenderQueueIndex);
			RendererToolkit::JsonHelper::optionalByteProperty(rapidJsonValuePass, "MaximumRenderQueueIndex", passScene.maximumRenderQueueIndex);
			RendererToolkit::JsonHelper::optionalBooleanProperty(rapidJsonValuePass, "TransparentPass", passScene.transparentPass);
			RendererToolkit::JsonHelper::mandatoryStringIdProperty(rapidJsonValuePass, "MaterialTechnique", passScene.materialTechniqueId);

			// Sanity checks
			if (passScene.maximumRenderQueueIndex < passScene.minimumRenderQueueIndex)
			{
				throw std::runtime_error("The maximum render queue index must be equal or greater as the minimum render queue index");
			}
		}

		uint32_t getRenderTargetTextureSize(const rapidjson::Value& rapidJsonValueRenderTargetTexture, const char* propertyName, const char* defaultValue)
		{
			uint32_t size = RendererRuntime::getInvalid<uint32_t>();
			if (rapidJsonValueRenderTargetTexture.HasMember(propertyName))
			{
				const char* valueAsString = rapidJsonValueRenderTargetTexture[propertyName].GetString();
				if (strcmp(valueAsString, defaultValue) != 0)
				{
					size = static_cast<uint32_t>(std::atoi(valueAsString));
				}
			}
			return size;
		}

		uint32_t getForEachInstructionParameters(const std::string& instructionAsString, std::string& scopedIterationCounterVariable)
		{
			// "@foreach(<number of iterations>, <scoped iteration counter variable>)" (same syntax as in "RendererRuntime::ShaderBuilder")
			std::vector<std::string> elements;
			RendererToolkit::StringHelper::splitString(instructionAsString.substr(9), ",)", elements);
			if (elements.size() != 2)
			{
				throw std::runtime_error("Compositor node: \"@foreach(<number of iterations>, <scoped iteration counter variable>)\" requires two parameters. Given instruction string: \"" + instructionAsString + '\"');
			}
			scopedIterationCounterVariable = elements[1];

			// Return the number of iterations
			return static_cast<uint32_t>(std::atoi(elements[0].c_str()));
		}

		uint32_t getNumberOfTargets(const rapidjson::Value& rapidJsonValueTargets)
		{
			// We can't just return "rapidJsonValueTargets.MemberCount()" since there might be "@foreach"-instructions
			uint32_t numberOfTargets = 0;
			for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorTargets = rapidJsonValueTargets.MemberBegin(); rapidJsonMemberIteratorTargets != rapidJsonValueTargets.MemberEnd(); ++rapidJsonMemberIteratorTargets)
			{
				const std::string targetName = rapidJsonMemberIteratorTargets->name.GetString();

				// Check for instruction "@foreach(<number of iterations>, <scoped iteration counter variable>)" (same syntax as in "RendererRuntime::ShaderBuilder")
				if (strncmp(targetName.c_str(), "@foreach(", 9) == 0)
				{
					// Get the two "@foreach"-parameters
					std::string scopedIterationCounterVariable;
					const uint32_t numberOfIterations = getForEachInstructionParameters(targetName, scopedIterationCounterVariable);

					// Unroll the loop
					for (uint32_t i = 0; i < numberOfIterations; ++i)
					{
						numberOfTargets += getNumberOfTargets(rapidJsonMemberIteratorTargets->value);
					}
				}
				else
				{
					++numberOfTargets;
				}
			}
			return numberOfTargets;
		}

		void processTargets(const RendererToolkit::IAssetCompiler::Input& input, const std::unordered_set<uint32_t>& compositorChannelIds, std::unordered_set<uint32_t>& renderTargetTextureAssetIds, const std::unordered_set<uint32_t>& compositorFramebufferIds, const rapidjson::Value& rapidJsonValueTargets, RendererRuntime::IFile& file)
		{
			for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorTargets = rapidJsonValueTargets.MemberBegin(); rapidJsonMemberIteratorTargets != rapidJsonValueTargets.MemberEnd(); ++rapidJsonMemberIteratorTargets)
			{
				const std::string targetName = rapidJsonMemberIteratorTargets->name.GetString();

				// Check for instruction "@foreach(<number of iterations>, <scoped iteration counter variable>)" (same syntax as in "RendererRuntime::ShaderBuilder")
				if (strncmp(targetName.c_str(), "@foreach(", 9) == 0)
				{
					// Get the two "@foreach"-parameters
					std::string scopedIterationCounterVariable;
					const uint32_t numberOfIterations = getForEachInstructionParameters(targetName, scopedIterationCounterVariable);
					// TODO(co) "scopedIterationCounterVariable" is currently unused

					// Unroll the loop
					for (uint32_t i = 0; i < numberOfIterations; ++i)
					{
						processTargets(input, compositorChannelIds, renderTargetTextureAssetIds, compositorFramebufferIds, rapidJsonMemberIteratorTargets->value, file);
					}
				}
				else
				{
					const rapidjson::Value& rapidJsonValueTarget = rapidJsonMemberIteratorTargets->value;
					const rapidjson::Value& rapidJsonValuePasses = rapidJsonValueTarget;	// A render target only contains passes, keep this variable to make the content more clear

					{ // Write down the compositor resource node target
					  // -> Render target might be compositor channel (external interconnection) or compositor framebuffer (node internal processing)
						const uint32_t id = RendererRuntime::StringId::calculateFNV(targetName.c_str());
						RendererRuntime::v1CompositorNode::Target target;
						target.compositorChannelId	   = (compositorChannelIds.find(id) != compositorChannelIds.end()) ? id : RendererRuntime::getInvalid<uint32_t>();
						target.compositorFramebufferId = (compositorFramebufferIds.find(id) != compositorFramebufferIds.end()) ? id : RendererRuntime::getInvalid<uint32_t>();
						target.numberOfPasses		   = rapidJsonValuePasses.MemberCount();
						file.write(&target, sizeof(RendererRuntime::v1CompositorNode::Target));
					}

					// Write down the compositor resource node target passes
					for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorPasses = rapidJsonValuePasses.MemberBegin(); rapidJsonMemberIteratorPasses != rapidJsonValuePasses.MemberEnd(); ++rapidJsonMemberIteratorPasses)
					{
						const rapidjson::Value& rapidJsonValuePass = rapidJsonMemberIteratorPasses->value;
						const RendererRuntime::CompositorPassTypeId compositorPassTypeId = RendererRuntime::StringId(rapidJsonMemberIteratorPasses->name.GetString());

						// Get the compositor resource node target pass type specific data number of bytes
						// TODO(co) Make this more generic via compositor pass factory
						uint32_t numberOfBytes = 0;
						RendererRuntime::MaterialProperties::SortedPropertyVector sortedMaterialPropertyVector;
						switch (compositorPassTypeId)
						{
							case RendererRuntime::CompositorResourcePassClear::TYPE_ID:
								numberOfBytes = sizeof(RendererRuntime::v1CompositorNode::PassClear);
								break;

							case RendererRuntime::CompositorResourcePassVrHiddenAreaMesh::TYPE_ID:
								numberOfBytes = sizeof(RendererRuntime::v1CompositorNode::PassVrHiddenAreaMesh);
								break;

							case RendererRuntime::CompositorResourcePassScene::TYPE_ID:
								numberOfBytes = sizeof(RendererRuntime::v1CompositorNode::PassScene);
								break;

							case RendererRuntime::CompositorResourcePassShadowMap::TYPE_ID:
								numberOfBytes = sizeof(RendererRuntime::v1CompositorNode::PassShadowMap);
								break;

							case RendererRuntime::CompositorResourcePassResolveMultisample::TYPE_ID:
								numberOfBytes = sizeof(RendererRuntime::v1CompositorNode::PassResolveMultisample);
								break;

							case RendererRuntime::CompositorResourcePassCopy::TYPE_ID:
								numberOfBytes = sizeof(RendererRuntime::v1CompositorNode::PassCopy);
								break;

							case RendererRuntime::CompositorResourcePassGenerateMipmaps::TYPE_ID:
								numberOfBytes = sizeof(RendererRuntime::v1CompositorNode::PassGenerateMipmaps);
								break;

							case RendererRuntime::CompositorResourcePassCompute::TYPE_ID:
								fillSortedMaterialPropertyVector(input, renderTargetTextureAssetIds, rapidJsonMemberIteratorPasses->value, sortedMaterialPropertyVector);
								numberOfBytes = static_cast<uint32_t>(sizeof(RendererRuntime::v1CompositorNode::PassCompute) + sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
								break;

							case RendererRuntime::CompositorResourcePassDebugGui::TYPE_ID:
								fillSortedMaterialPropertyVector(input, renderTargetTextureAssetIds, rapidJsonMemberIteratorPasses->value, sortedMaterialPropertyVector);
								numberOfBytes = static_cast<uint32_t>(sizeof(RendererRuntime::v1CompositorNode::PassDebugGui) + sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
								break;
						}

						{ // Write down the compositor resource node target pass header
							RendererRuntime::v1CompositorNode::PassHeader passHeader;
							passHeader.compositorPassTypeId = compositorPassTypeId;
							passHeader.numberOfBytes		= numberOfBytes;
							file.write(&passHeader, sizeof(RendererRuntime::v1CompositorNode::PassHeader));
						}

						// Write down the compositor resource node target pass type specific data, if there is any
						if (0 != numberOfBytes)
						{
							switch (compositorPassTypeId)
							{
								case RendererRuntime::CompositorResourcePassClear::TYPE_ID:
								{
									RendererRuntime::v1CompositorNode::PassClear passClear;
									readPass(rapidJsonValuePass, passClear);

									// Read properties
									RendererToolkit::JsonHelper::optionalClearFlagsProperty(rapidJsonValuePass, "Flags", passClear.flags);
									RendererToolkit::JsonHelper::optionalFloatNProperty(rapidJsonValuePass, "Color", passClear.color, 4);
									RendererToolkit::JsonHelper::optionalFloatProperty(rapidJsonValuePass, "Z", passClear.z);
									RendererToolkit::JsonHelper::optionalIntegerProperty(rapidJsonValuePass, "Stencil", passClear.stencil);
									if (0 == passClear.flags)
									{
										throw std::runtime_error("The clear compositor resource pass flags must not be null");
									}

									// Write down
									file.write(&passClear, sizeof(RendererRuntime::v1CompositorNode::PassClear));
									break;
								}

								case RendererRuntime::CompositorResourcePassVrHiddenAreaMesh::TYPE_ID:
								{
									RendererRuntime::v1CompositorNode::PassVrHiddenAreaMesh passVrHiddenAreaMesh;
									readPass(rapidJsonValuePass, passVrHiddenAreaMesh);

									// Read properties
									RendererToolkit::JsonHelper::optionalClearFlagsProperty(rapidJsonValuePass, "Flags", passVrHiddenAreaMesh.flags);
									RendererToolkit::JsonHelper::optionalIntegerProperty(rapidJsonValuePass, "Stencil", passVrHiddenAreaMesh.stencil);
									if (0 == passVrHiddenAreaMesh.flags)
									{
										throw std::runtime_error("The VR hidden area mesh compositor resource pass flags must not be null");
									}
									if ((passVrHiddenAreaMesh.flags & Renderer::ClearFlag::COLOR) != 0)
									{
										throw std::runtime_error("The VR hidden area mesh compositor resource pass doesn't support the color flag");
									}
									if ((passVrHiddenAreaMesh.flags & Renderer::ClearFlag::STENCIL) != 0)
									{
										throw std::runtime_error("TODO(co) The VR hidden area mesh compositor resource pass doesn't support the stencil flag, yet");
									}

									// Write down
									file.write(&passVrHiddenAreaMesh, sizeof(RendererRuntime::v1CompositorNode::PassVrHiddenAreaMesh));
									break;
								}

								case RendererRuntime::CompositorResourcePassScene::TYPE_ID:
								{
									RendererRuntime::v1CompositorNode::PassScene passScene;
									readPass(rapidJsonValuePass, passScene);
									readPassScene(rapidJsonValuePass, passScene);

									// Write down
									file.write(&passScene, sizeof(RendererRuntime::v1CompositorNode::PassScene));
									break;
								}

								case RendererRuntime::CompositorResourcePassShadowMap::TYPE_ID:
								{
									RendererRuntime::v1CompositorNode::PassShadowMap passShadowMap;
									readPass(rapidJsonValuePass, passShadowMap);
									readPassScene(rapidJsonValuePass, passShadowMap);
									RendererToolkit::JsonHelper::mandatoryAssetIdProperty(rapidJsonValuePass, "Texture", passShadowMap.textureAssetId);
									passShadowMap.depthToExponentialVarianceMaterialBlueprintAssetId = RendererToolkit::JsonHelper::getCompiledAssetId(input, rapidJsonValuePass, "DepthToExponentialVarianceMaterialBlueprint");
									passShadowMap.blurMaterialBlueprintAssetId = RendererToolkit::JsonHelper::getCompiledAssetId(input, rapidJsonValuePass, "BlurMaterialBlueprint");
									renderTargetTextureAssetIds.insert(passShadowMap.textureAssetId);

									// Write down
									file.write(&passShadowMap, sizeof(RendererRuntime::v1CompositorNode::PassShadowMap));
									break;
								}

								case RendererRuntime::CompositorResourcePassResolveMultisample::TYPE_ID:
								{
									RendererRuntime::v1CompositorNode::PassResolveMultisample passResolveMultisample;
									readPass(rapidJsonValuePass, passResolveMultisample);
									RendererToolkit::JsonHelper::mandatoryStringIdProperty(rapidJsonValuePass, "SourceMultisampleFramebuffer", passResolveMultisample.sourceMultisampleCompositorFramebufferId);
									if (compositorFramebufferIds.find(passResolveMultisample.sourceMultisampleCompositorFramebufferId) == compositorFramebufferIds.end())
									{
										throw std::runtime_error(std::string("Source multisample framebuffer \"") + rapidJsonValuePass["SourceMultisampleFramebuffer"].GetString() + "\" is unknown");
									}
									file.write(&passResolveMultisample, sizeof(RendererRuntime::v1CompositorNode::PassResolveMultisample));
									break;
								}

								case RendererRuntime::CompositorResourcePassCopy::TYPE_ID:
								{
									RendererRuntime::v1CompositorNode::PassCopy passCopy;
									readPass(rapidJsonValuePass, passCopy);
									RendererToolkit::JsonHelper::mandatoryStringIdProperty(rapidJsonValuePass, "DestinationTexture", passCopy.destinationTextureAssetId);
									RendererToolkit::JsonHelper::mandatoryStringIdProperty(rapidJsonValuePass, "SourceTexture", passCopy.sourceTextureAssetId);
									if (renderTargetTextureAssetIds.find(passCopy.destinationTextureAssetId) == renderTargetTextureAssetIds.end())
									{
										throw std::runtime_error(std::string("Destination texture asset ID \"") + rapidJsonValuePass["DestinationTexture"].GetString() + "\" is unknown");
									}
									if (renderTargetTextureAssetIds.find(passCopy.sourceTextureAssetId) == renderTargetTextureAssetIds.end())
									{
										throw std::runtime_error(std::string("Source texture asset ID \"") + rapidJsonValuePass["SourceTexture"].GetString() + "\" is unknown");
									}
									file.write(&passCopy, sizeof(RendererRuntime::v1CompositorNode::PassCopy));
									break;
								}

								case RendererRuntime::CompositorResourcePassGenerateMipmaps::TYPE_ID:
								{
									RendererRuntime::v1CompositorNode::PassGenerateMipmaps passGenerateMipmaps;
									readPass(rapidJsonValuePass, passGenerateMipmaps);
									RendererToolkit::JsonHelper::mandatoryStringIdProperty(rapidJsonValuePass, "DepthTexture", passGenerateMipmaps.depthTextureAssetId);
									passGenerateMipmaps.materialBlueprintAssetId = RendererToolkit::JsonHelper::getCompiledAssetId(input, rapidJsonValuePass, "MaterialBlueprint");
									file.write(&passGenerateMipmaps, sizeof(RendererRuntime::v1CompositorNode::PassGenerateMipmaps));
									break;
								}

								case RendererRuntime::CompositorResourcePassCompute::TYPE_ID:
								{
									RendererRuntime::v1CompositorNode::PassCompute passCompute;
									readPass(rapidJsonValuePass, passCompute);
									readPassCompute(input, sortedMaterialPropertyVector, rapidJsonValuePass, true, passCompute);

									// Write down
									file.write(&passCompute, sizeof(RendererRuntime::v1CompositorNode::PassCompute));
									if (!sortedMaterialPropertyVector.empty())
									{
										// Write down all material properties
										file.write(sortedMaterialPropertyVector.data(), sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
									}
									break;
								}

								case RendererRuntime::CompositorResourcePassDebugGui::TYPE_ID:
								{
									// The material definition is not mandatory for the debug GUI, if nothing is defined the fixed build in renderer configuration resources will be used instead
									RendererRuntime::v1CompositorNode::PassDebugGui passDebugGui;
									readPass(rapidJsonValuePass, passDebugGui);
									readPassCompute(input, sortedMaterialPropertyVector, rapidJsonValuePass, false, passDebugGui);

									// Write down
									file.write(&passDebugGui, sizeof(RendererRuntime::v1CompositorNode::PassDebugGui));
									if (!sortedMaterialPropertyVector.empty())
									{
										// Write down all material properties
										file.write(sortedMaterialPropertyVector.data(), sizeof(RendererRuntime::MaterialProperty) * sortedMaterialPropertyVector.size());
									}
									break;
								}
							}
						}
					}
				}
			}
		}

		void optionalRenderTargetTexturesFlagsProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, uint8_t& flags)
		{
			if (rapidJsonValue.HasMember(propertyName))
			{
				flags = 0;
				std::vector<std::string> flagsAsString;
				RendererToolkit::StringHelper::splitString(rapidJsonValue[propertyName].GetString(), '|', flagsAsString);
				if (!flagsAsString.empty())
				{
					// Define helper macros
					#define IF_VALUE(name)			 if (flagAsString == #name) value = RendererRuntime::RenderTargetTextureSignature::Flag::name;
					#define ELSE_IF_VALUE(name) else if (flagAsString == #name) value = RendererRuntime::RenderTargetTextureSignature::Flag::name;

					// Evaluate flags
					const size_t numberOfFlags = flagsAsString.size();
					for (size_t i = 0; i < numberOfFlags; ++i)
					{
						// Get flag as string
						std::string flagAsString = flagsAsString[i];
						RendererToolkit::StringHelper::trimWhitespaceCharacters(flagAsString);

						// Evaluate value
						uint8_t value = 0;
						IF_VALUE(UNORDERED_ACCESS)
						ELSE_IF_VALUE(SHADER_RESOURCE)
						ELSE_IF_VALUE(RENDER_TARGET)
						ELSE_IF_VALUE(ALLOW_MULTISAMPLE)
						ELSE_IF_VALUE(GENERATE_MIPMAPS)
						ELSE_IF_VALUE(ALLOW_RESOLUTION_SCALE)
						else
						{
							throw std::runtime_error('\"' + std::string(propertyName) + "\" doesn't know the flag " + flagAsString + ". Must be \"UNORDERED_ACCESS\", \"SHADER_RESOURCE\", \"RENDER_TARGET\", \"ALLOW_MULTISAMPLE\", \"GENERATE_MIPMAPS\" or \"ALLOW_RESOLUTION_SCALE\".");
						}

						// Apply value
						flags |= value;
					}

					// Undefine helper macros
					#undef IF_VALUE
					#undef ELSE_IF_VALUE
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
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	CompositorNodeAssetCompiler::CompositorNodeAssetCompiler()
	{
		// Nothing here
	}

	CompositorNodeAssetCompiler::~CompositorNodeAssetCompiler()
	{
		// Nothing here
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererToolkit::IAssetCompiler methods ]
	//[-------------------------------------------------------]
	AssetCompilerTypeId CompositorNodeAssetCompiler::getAssetCompilerTypeId() const
	{
		return TYPE_ID;
	}

	bool CompositorNodeAssetCompiler::checkIfChanged(const Input& input, const Configuration& configuration) const
	{
		// Let the cache manager check whether or not the files have been changed in order to speed up later checks and to support dependency tracking
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + configuration.rapidJsonDocumentAsset["Asset"]["CompositorNodeAssetCompiler"]["InputFile"].GetString();
		const std::string virtualOutputAssetFilename = input.virtualAssetOutputDirectory + '/' + std_filesystem::path(input.virtualAssetFilename).stem().generic_string() + ".compositor_node";
		return input.cacheManager.checkIfFileIsModified(configuration.rendererTarget, input.virtualAssetFilename, {virtualInputFilename}, virtualOutputAssetFilename, RendererRuntime::v1CompositorNode::FORMAT_VERSION);
	}

	void CompositorNodeAssetCompiler::compile(const Input& input, const Configuration& configuration, Output& output)
	{
		// Get relevant data
		const rapidjson::Value& rapidJsonValueAsset = configuration.rapidJsonDocumentAsset["Asset"];
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + rapidJsonValueAsset["CompositorNodeAssetCompiler"]["InputFile"].GetString();
		const std::string assetName = std_filesystem::path(input.virtualAssetFilename).stem().generic_string();
		const std::string virtualOutputAssetFilename = input.virtualAssetOutputDirectory + '/' + assetName + ".compositor_node";

		// Ask the cache manager whether or not we need to compile the source file (e.g. source changed or target not there)
		CacheManager::CacheEntries cacheEntries;
		if (input.cacheManager.needsToBeCompiled(configuration.rendererTarget, input.virtualAssetFilename, virtualInputFilename, virtualOutputAssetFilename, RendererRuntime::v1CompositorNode::FORMAT_VERSION, cacheEntries))
		{
			RendererRuntime::MemoryFile memoryFile(0, 4096);

			{ // Compositor node
				// Parse JSON
				rapidjson::Document rapidJsonDocument;
				JsonHelper::loadDocumentByFilename(input.context.getFileManager(), virtualInputFilename, "CompositorNodeAsset", "1", rapidJsonDocument);

				// Mandatory main sections of the compositor node
				const rapidjson::Value& rapidJsonValueCompositorNodeAsset = rapidJsonDocument["CompositorNodeAsset"];
				const rapidjson::Value& rapidJsonValueInputChannels = rapidJsonValueCompositorNodeAsset["InputChannels"];
				const rapidjson::Value& rapidJsonValueTargets = rapidJsonValueCompositorNodeAsset["Targets"];
				const rapidjson::Value& rapidJsonValueOutputChannels = rapidJsonValueCompositorNodeAsset["OutputChannels"];

				{ // Write down the compositor node resource header
					RendererRuntime::v1CompositorNode::CompositorNodeHeader compositorNodeHeader;
					compositorNodeHeader.numberOfInputChannels		  = rapidJsonValueInputChannels.MemberCount();
					compositorNodeHeader.numberOfRenderTargetTextures = rapidJsonValueCompositorNodeAsset.HasMember("RenderTargetTextures") ? rapidJsonValueCompositorNodeAsset["RenderTargetTextures"].MemberCount() : 0;
					compositorNodeHeader.numberOfFramebuffers		  = rapidJsonValueCompositorNodeAsset.HasMember("Framebuffers") ? rapidJsonValueCompositorNodeAsset["Framebuffers"].MemberCount() : 0;
					compositorNodeHeader.numberOfTargets			  = ::detail::getNumberOfTargets(rapidJsonValueTargets);
					compositorNodeHeader.numberOfOutputChannels		  = rapidJsonValueOutputChannels.MemberCount();
					memoryFile.write(&compositorNodeHeader, sizeof(RendererRuntime::v1CompositorNode::CompositorNodeHeader));
				}

				// Write down the compositor resource node input channels
				std::unordered_set<uint32_t> compositorChannelIds;	// "RendererRuntime::CompositorChannelId"-type
				for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorInputChannels = rapidJsonValueInputChannels.MemberBegin(); rapidJsonMemberIteratorInputChannels != rapidJsonValueInputChannels.MemberEnd(); ++rapidJsonMemberIteratorInputChannels)
				{
					RendererRuntime::v1CompositorNode::Channel channel;
					channel.id = RendererRuntime::StringId(rapidJsonMemberIteratorInputChannels->value.GetString());
					memoryFile.write(&channel, sizeof(RendererRuntime::v1CompositorNode::Channel));

					// Remember that there's a compositor channel with this ID
					compositorChannelIds.insert(channel.id);
				}

				// Write down the compositor render target textures
				std::unordered_set<uint32_t> renderTargetTextureAssetIds = input.defaultTextureAssetIds;
				std::unordered_map<uint32_t, RendererRuntime::RenderTargetTextureSignature> renderTargetTextureSignatures;
				if (rapidJsonValueCompositorNodeAsset.HasMember("RenderTargetTextures"))
				{
					const rapidjson::Value& rapidJsonValueRenderTargetTextures = rapidJsonValueCompositorNodeAsset["RenderTargetTextures"];
					for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorRenderTargetTextures = rapidJsonValueRenderTargetTextures.MemberBegin(); rapidJsonMemberIteratorRenderTargetTextures != rapidJsonValueRenderTargetTextures.MemberEnd(); ++rapidJsonMemberIteratorRenderTargetTextures)
					{
						RendererRuntime::v1CompositorNode::RenderTargetTexture renderTargetTexture;
						renderTargetTexture.assetId = StringHelper::getAssetIdByString(rapidJsonMemberIteratorRenderTargetTextures->name.GetString());
						{ // Render target texture signature
							const rapidjson::Value& rapidJsonValueRenderTargetTexture = rapidJsonMemberIteratorRenderTargetTextures->value;

							// Width and height
							const uint32_t width = ::detail::getRenderTargetTextureSize(rapidJsonValueRenderTargetTexture, "Width", "TARGET_WIDTH");
							const uint32_t height = ::detail::getRenderTargetTextureSize(rapidJsonValueRenderTargetTexture, "Height", "TARGET_HEIGHT");

							// Texture format
							const Renderer::TextureFormat::Enum textureFormat = JsonHelper::mandatoryTextureFormat(rapidJsonValueRenderTargetTexture);

							// Flags
							uint8_t flags = (RendererRuntime::RenderTargetTextureSignature::Flag::SHADER_RESOURCE | RendererRuntime::RenderTargetTextureSignature::Flag::RENDER_TARGET);
							if (RendererRuntime::isInvalid(width) || RendererRuntime::isInvalid(height))
							{
								flags |= RendererRuntime::RenderTargetTextureSignature::Flag::ALLOW_RESOLUTION_SCALE;
							}
							::detail::optionalRenderTargetTexturesFlagsProperty(rapidJsonValueRenderTargetTexture, "Flags", flags);
							if (RendererRuntime::isValid(width) && RendererRuntime::isValid(height) && (flags & RendererRuntime::RenderTargetTextureSignature::Flag::ALLOW_RESOLUTION_SCALE) != 0)
							{
								throw std::runtime_error(std::string("Render target texture \"") + rapidJsonMemberIteratorRenderTargetTextures->name.GetString() + "\" has a fixed defined width and height, usage of \"ALLOW_RESOLUTION_SCALE\" flag is not allowed for this use-case");
							}

							// Width scale
							float widthScale = 1.0f;
							if (RendererRuntime::isValid(width) && rapidJsonValueRenderTargetTexture.HasMember("WidthScale"))
							{
								throw std::runtime_error(std::string("Render target texture \"") + rapidJsonMemberIteratorRenderTargetTextures->name.GetString() + "\" has a fixed defined width, usage of \"WidthScale\" is not allowed for this use-case");
							}
							JsonHelper::optionalFloatProperty(rapidJsonValueRenderTargetTexture, "WidthScale", widthScale);

							// Height scale
							float heightScale = 1.0f;
							if (RendererRuntime::isValid(height) && rapidJsonValueRenderTargetTexture.HasMember("HeightScale"))
							{
								throw std::runtime_error(std::string("Render target texture \"") + rapidJsonMemberIteratorRenderTargetTextures->name.GetString() + "\" has a fixed defined height, usage of \"HeightScale\" is not allowed for this use-case");
							}
							JsonHelper::optionalFloatProperty(rapidJsonValueRenderTargetTexture, "HeightScale", heightScale);

							// Ease of use scale for width as well as height
							if (rapidJsonValueRenderTargetTexture.HasMember("Scale") && (rapidJsonValueRenderTargetTexture.HasMember("WidthScale") || rapidJsonValueRenderTargetTexture.HasMember("HeightScale")))
							{
								throw std::runtime_error(std::string("Render target texture \"") + rapidJsonMemberIteratorRenderTargetTextures->name.GetString() + "\" has an already defined width and/or height scale, usage of \"Scale\" is not allowed for this use-case");
							}
							float scale = 1.0f;
							JsonHelper::optionalFloatProperty(rapidJsonValueRenderTargetTexture, "Scale", scale);
							if (!rapidJsonValueRenderTargetTexture.HasMember("WidthScale"))
							{
								widthScale = scale;
							}
							if (!rapidJsonValueRenderTargetTexture.HasMember("HeightScale"))
							{
								heightScale = scale;
							}

							// Write render target texture signature
							// TODO(co) Add sanity checks to be able to detect editing errors (compressed formats are not supported nor unknown formats, check for name conflicts with channels, unused render target textures etc.)
							renderTargetTexture.renderTargetTextureSignature = RendererRuntime::RenderTargetTextureSignature(width, height, textureFormat, flags, widthScale, heightScale);
							renderTargetTextureSignatures.emplace(renderTargetTexture.assetId, renderTargetTexture.renderTargetTextureSignature);
						}
						memoryFile.write(&renderTargetTexture, sizeof(RendererRuntime::v1CompositorNode::RenderTargetTexture));

						// Remember that there's a render target texture with this asset ID
						renderTargetTextureAssetIds.insert(renderTargetTexture.assetId);
					}
				}

				// Write down the compositor framebuffers
				std::unordered_set<uint32_t> compositorFramebufferIds;	// "RendererRuntime::CompositorFramebufferId"-type
				if (rapidJsonValueCompositorNodeAsset.HasMember("Framebuffers"))
				{
					const rapidjson::Value& rapidJsonValueFramebuffers = rapidJsonValueCompositorNodeAsset["Framebuffers"];
					for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorFramebuffers = rapidJsonValueFramebuffers.MemberBegin(); rapidJsonMemberIteratorFramebuffers != rapidJsonValueFramebuffers.MemberEnd(); ++rapidJsonMemberIteratorFramebuffers)
					{
						RendererRuntime::v1CompositorNode::Framebuffer framebuffer;
						framebuffer.compositorFramebufferId = RendererRuntime::StringId(rapidJsonMemberIteratorFramebuffers->name.GetString());
						{ // Framebuffer signature
							const rapidjson::Value& rapidJsonValueFramebuffer = rapidJsonMemberIteratorFramebuffers->value;

							// Optional color framebuffer attachment
							uint8_t numberOfColorFramebufferAttachments = 0;
							RendererRuntime::FramebufferSignatureAttachment colorFramebufferSignatureAttachments[8];
							if (rapidJsonValueFramebuffer.HasMember("ColorTextures") && rapidJsonValueFramebuffer.HasMember("ColorAttachments"))
							{
								throw std::runtime_error("Framebuffer \"" + std::string(rapidJsonMemberIteratorFramebuffers->name.GetString()) + "\" is using \"ColorTextures\" as well as \"ColorAttachments\", only one can be used at one and the same time");
							}
							if (rapidJsonValueFramebuffer.HasMember("ColorAttachments"))
							{
								// "ColorAttachments" for custom mipmap index and layer index, useful for example for low resolution particles or distortion rendering
								const rapidjson::Value& rapidJsonValueFramebufferColorAttachments = rapidJsonValueFramebuffer["ColorAttachments"];
								numberOfColorFramebufferAttachments = static_cast<uint8_t>(rapidJsonValueFramebufferColorAttachments.Size());
								for (uint8_t i = 0; i < numberOfColorFramebufferAttachments; ++i)
								{
									const rapidjson::Value& rapidJsonValueFramebufferColorAttachment = rapidJsonValueFramebufferColorAttachments[i];
									RendererRuntime::FramebufferSignatureAttachment& colorFramebufferSignatureAttachment = colorFramebufferSignatureAttachments[i];
									const RendererRuntime::AssetId textureAssetId = StringHelper::getAssetIdByString(rapidJsonValueFramebufferColorAttachment["ColorTexture"].GetString());
									if (RendererRuntime::isValid(textureAssetId) && renderTargetTextureAssetIds.find(textureAssetId) == renderTargetTextureAssetIds.end())
									{
										throw std::runtime_error(std::string("Color texture \"") + rapidJsonValueFramebufferColorAttachment["ColorTexture"].GetString() + "\" at index " + std::to_string(i) + " of framebuffer \"" + rapidJsonMemberIteratorFramebuffers->name.GetString() + "\" is unknown");
									}
									if ((renderTargetTextureSignatures.find(textureAssetId)->second.getFlags() & RendererRuntime::RenderTargetTextureSignature::Flag::RENDER_TARGET) == 0)
									{
										throw std::runtime_error(std::string("Color texture \"") + rapidJsonValueFramebufferColorAttachment["ColorTexture"].GetString() + "\" at index " + std::to_string(i) + " of framebuffer \"" + rapidJsonMemberIteratorFramebuffers->name.GetString() + "\" has no \"RENDER_TARGET\" flag set");
									}
									colorFramebufferSignatureAttachment.textureAssetId = textureAssetId;
									JsonHelper::optionalIntegerProperty(rapidJsonValueFramebufferColorAttachment, "MipmapIndex", colorFramebufferSignatureAttachment.mipmapIndex);
									JsonHelper::optionalIntegerProperty(rapidJsonValueFramebufferColorAttachment, "LayerIndex", colorFramebufferSignatureAttachment.layerIndex);
								}
							}
							else if (rapidJsonValueFramebuffer.HasMember("ColorTextures"))
							{
								// Ease-of-use "ColorTextures" for zero mipmap index and layer index, sufficient most of the time
								const rapidjson::Value& rapidJsonValueFramebufferColorTextures = rapidJsonValueFramebuffer["ColorTextures"];
								numberOfColorFramebufferAttachments = static_cast<uint8_t>(rapidJsonValueFramebufferColorTextures.Size());
								for (uint8_t i = 0; i < numberOfColorFramebufferAttachments; ++i)
								{
									const RendererRuntime::AssetId textureAssetId = StringHelper::getAssetIdByString(rapidJsonValueFramebufferColorTextures[i].GetString());
									if (RendererRuntime::isValid(textureAssetId) && renderTargetTextureAssetIds.find(textureAssetId) == renderTargetTextureAssetIds.end())
									{
										throw std::runtime_error(std::string("Color texture \"") + rapidJsonValueFramebufferColorTextures[i].GetString() + "\" at index " + std::to_string(i) + " of framebuffer \"" + rapidJsonMemberIteratorFramebuffers->name.GetString() + "\" is unknown");
									}
									if ((renderTargetTextureSignatures.find(textureAssetId)->second.getFlags() & RendererRuntime::RenderTargetTextureSignature::Flag::RENDER_TARGET) == 0)
									{
										throw std::runtime_error(std::string("Color texture \"") + rapidJsonValueFramebufferColorTextures[i].GetString() + "\" at index " + std::to_string(i) + " of framebuffer \"" + rapidJsonMemberIteratorFramebuffers->name.GetString() + "\" has no \"RENDER_TARGET\" flag set");
									}
									colorFramebufferSignatureAttachments[i].textureAssetId = textureAssetId;
								}
							}

							// Optional depth stencil framebuffer attachment
							uint32_t depthStencilTextureAssetId = RendererRuntime::getInvalid<uint32_t>();
							RendererRuntime::FramebufferSignatureAttachment depthStencilFramebufferSignatureAttachment;
							if (rapidJsonValueFramebuffer.HasMember("DepthStencilTexture") && rapidJsonValueFramebuffer.HasMember("DepthStencilAttachment"))
							{
								throw std::runtime_error("Framebuffer \"" + std::string(rapidJsonMemberIteratorFramebuffers->name.GetString()) + "\" is using \"DepthStencilTexture\" as well as \"DepthStencilAttachment\", only one can be used at one and the same time");
							}
							if (rapidJsonValueFramebuffer.HasMember("DepthStencilAttachment"))
							{
								// "DepthStencilAttachment" for custom mipmap index and layer index, useful for example for low resolution particles or distortion rendering
								const rapidjson::Value& rapidJsonValueDepthStencilAttachment = rapidJsonValueFramebuffer["DepthStencilAttachment"];
								depthStencilTextureAssetId = StringHelper::getAssetIdByString(rapidJsonValueDepthStencilAttachment["DepthStencilTexture"].GetString());
								JsonHelper::optionalIntegerProperty(rapidJsonValueDepthStencilAttachment, "MipmapIndex", depthStencilFramebufferSignatureAttachment.mipmapIndex);
								JsonHelper::optionalIntegerProperty(rapidJsonValueDepthStencilAttachment, "LayerIndex", depthStencilFramebufferSignatureAttachment.layerIndex);
							}
							else
							{
								// Ease-of-use "DepthStencilTexture" for zero mipmap index and layer index, sufficient most of the time
								depthStencilTextureAssetId = rapidJsonValueFramebuffer.HasMember("DepthStencilTexture") ? StringHelper::getAssetIdByString(rapidJsonValueFramebuffer["DepthStencilTexture"].GetString()) : RendererRuntime::getInvalid<RendererRuntime::AssetId>();
							}
							depthStencilFramebufferSignatureAttachment.textureAssetId = depthStencilTextureAssetId;
							if (RendererRuntime::isValid(depthStencilTextureAssetId) && renderTargetTextureAssetIds.find(depthStencilTextureAssetId) == renderTargetTextureAssetIds.end())
							{
								throw std::runtime_error(std::string("Depth stencil texture \"") + rapidJsonValueFramebuffer["DepthStencilTexture"].GetString() + "\" of framebuffer \"" + rapidJsonMemberIteratorFramebuffers->name.GetString() + "\" is unknown");
							}
							if ((renderTargetTextureSignatures.find(depthStencilTextureAssetId)->second.getFlags() & RendererRuntime::RenderTargetTextureSignature::Flag::RENDER_TARGET) == 0)
							{
								throw std::runtime_error(std::string("Depth stencil texture \"") + rapidJsonValueFramebuffer["DepthStencilTexture"].GetString() + "\" of framebuffer \"" + rapidJsonMemberIteratorFramebuffers->name.GetString() + "\" has no \"RENDER_TARGET\" flag set");
							}

							// Write framebuffer signature
							// TODO(co) Add sanity checks to be able to detect editing errors (check for name conflicts with channels, unused framebuffers etc.)
							framebuffer.framebufferSignature = RendererRuntime::FramebufferSignature(numberOfColorFramebufferAttachments, colorFramebufferSignatureAttachments, depthStencilFramebufferSignatureAttachment);
						}
						memoryFile.write(&framebuffer, sizeof(RendererRuntime::v1CompositorNode::Framebuffer));

						// Remember that there's a compositor framebuffer with this ID
						compositorFramebufferIds.insert(framebuffer.compositorFramebufferId);
					}
				}

				// Write down the compositor resource node targets
				::detail::processTargets(input, compositorChannelIds, renderTargetTextureAssetIds, compositorFramebufferIds, rapidJsonValueTargets, memoryFile);

				// Write down the compositor resource node output channels
				for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorOutputChannels = rapidJsonValueOutputChannels.MemberBegin(); rapidJsonMemberIteratorOutputChannels != rapidJsonValueOutputChannels.MemberEnd(); ++rapidJsonMemberIteratorOutputChannels)
				{
					RendererRuntime::v1CompositorNode::Channel channel;
					channel.id = RendererRuntime::StringId(rapidJsonMemberIteratorOutputChannels->name.GetString());
					memoryFile.write(&channel, sizeof(RendererRuntime::v1CompositorNode::Channel));
				}
			}

			// Write LZ4 compressed output
			memoryFile.writeLz4CompressedDataByVirtualFilename(RendererRuntime::v1CompositorNode::FORMAT_TYPE, RendererRuntime::v1CompositorNode::FORMAT_VERSION, input.context.getFileManager(), virtualOutputAssetFilename.c_str());

			// Store new cache entries or update existing ones
			input.cacheManager.storeOrUpdateCacheEntries(cacheEntries);
		}

		{ // Update the output asset package
			const std::string assetCategory = rapidJsonValueAsset["AssetMetadata"]["AssetCategory"].GetString();
			const std::string assetIdAsString = input.projectName + "/CompositorNode/" + assetCategory + '/' + assetName;
			outputAsset(input.context.getFileManager(), assetIdAsString, virtualOutputAssetFilename, *output.outputAssetPackage);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
