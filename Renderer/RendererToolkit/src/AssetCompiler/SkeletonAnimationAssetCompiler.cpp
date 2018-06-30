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
#include "RendererToolkit/AssetCompiler/SkeletonAnimationAssetCompiler.h"
#include "RendererToolkit/Helper/AssimpLogStream.h"
#include "RendererToolkit/Helper/AssimpIOSystem.h"
#include "RendererToolkit/Helper/AssimpHelper.h"
#include "RendererToolkit/Helper/CacheManager.h"
#include "RendererToolkit/Helper/StringHelper.h"
#include "RendererToolkit/Helper/JsonHelper.h"
#include "RendererToolkit/Context.h"

#include <RendererRuntime/Asset/AssetPackage.h>
#include <RendererRuntime/Core/File/IFile.h>
#include <RendererRuntime/Core/File/MemoryFile.h>
#include <RendererRuntime/Core/File/IFileManager.h>
#include <RendererRuntime/Core/File/FileSystemHelper.h>
#include <RendererRuntime/Core/GetInvalid.h>
#include <RendererRuntime/Resource/SkeletonAnimation/SkeletonAnimationResource.h>
#include <RendererRuntime/Resource/SkeletonAnimation/Loader/SkeletonAnimationFileFormat.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator 'FORCE_32BIT' in switch of enum 'aiMetadataType' is not explicitly handled by a case label
	#include <assimp/scene.h>
	#include <assimp/Importer.hpp>
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
	SkeletonAnimationAssetCompiler::SkeletonAnimationAssetCompiler()
	{
		// Nothing here
	}

	SkeletonAnimationAssetCompiler::~SkeletonAnimationAssetCompiler()
	{
		// Nothing here
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererToolkit::IAssetCompiler methods ]
	//[-------------------------------------------------------]
	AssetCompilerTypeId SkeletonAnimationAssetCompiler::getAssetCompilerTypeId() const
	{
		return TYPE_ID;
	}

	bool SkeletonAnimationAssetCompiler::checkIfChanged(const Input& input, const Configuration& configuration) const
	{
		// Let the cache manager check whether or not the files have been changed in order to speed up later checks and to support dependency tracking
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + configuration.rapidJsonDocumentAsset["Asset"]["SkeletonAnimationAssetCompiler"]["InputFile"].GetString();
		const std::string virtualOutputAssetFilename = input.virtualAssetOutputDirectory + '/' + std_filesystem::path(input.virtualAssetFilename).stem().generic_string() + ".skeleton_animation";
		return input.cacheManager.checkIfFileIsModified(configuration.rendererTarget, input.virtualAssetFilename, {virtualInputFilename}, virtualOutputAssetFilename, RendererRuntime::v1SkeletonAnimation::FORMAT_VERSION);
	}

	void SkeletonAnimationAssetCompiler::compile(const Input& input, const Configuration& configuration, Output& output)
	{
		// Get relevant data
		const rapidjson::Value& rapidJsonValueAsset = configuration.rapidJsonDocumentAsset["Asset"];
		const rapidjson::Value& rapidJsonValueSkeletonAnimationAssetCompiler = rapidJsonValueAsset["SkeletonAnimationAssetCompiler"];
		const std::string virtualInputFilename = input.virtualAssetInputDirectory + '/' + rapidJsonValueSkeletonAnimationAssetCompiler["InputFile"].GetString();
		const std::string assetName = std_filesystem::path(input.virtualAssetFilename).stem().generic_string();
		const std::string virtualOutputAssetFilename = input.virtualAssetOutputDirectory + '/' + assetName + ".skeleton_animation";

		// Ask the cache manager whether or not we need to compile the source file (e.g. source changed or target not there)
		CacheManager::CacheEntries cacheEntries;
		if (input.cacheManager.needsToBeCompiled(configuration.rendererTarget, input.virtualAssetFilename, virtualInputFilename, virtualOutputAssetFilename, RendererRuntime::v1SkeletonAnimation::FORMAT_VERSION, cacheEntries))
		{
			RendererRuntime::MemoryFile memoryFile(0, 4096);

			// Create an instance of the Assimp importer class
			AssimpLogStream assimpLogStream;
			Assimp::Importer assimpImporter;
			assimpImporter.SetIOHandler(new AssimpIOSystem(input.context.getFileManager()));

			// Load the given mesh
			const aiScene* assimpScene = assimpImporter.ReadFile(virtualInputFilename.c_str(), AssimpHelper::getAssimpFlagsByRapidJsonValue(rapidJsonValueSkeletonAnimationAssetCompiler, "ImportFlags"));
			if (nullptr != assimpScene && nullptr != assimpScene->mRootNode)
			{
				// Read skeleton animation asset compiler configuration
				uint32_t animationIndex = RendererRuntime::getInvalid<uint32_t>();
				JsonHelper::optionalIntegerProperty(rapidJsonValueSkeletonAnimationAssetCompiler, "AnimationIndex", animationIndex);
				bool ignoreBoneScale = false;
				JsonHelper::optionalBooleanProperty(rapidJsonValueSkeletonAnimationAssetCompiler, "IgnoreBoneScale", ignoreBoneScale);

				// Get the Assimp animation instance to import
				// -> In case there are multiple animations stored inside the imported skeleton animation we must
				//    insist that the skeleton animation compiler gets supplied with the animation index to use
				// -> One skeleton animation assets contains one skeleton animation, everything else would make things more complicated in high-level animation systems
				if (!assimpScene->HasAnimations())
				{
					throw std::runtime_error("The input file \"" + virtualInputFilename + "\" contains no animations");
				}
				if (assimpScene->mNumAnimations > 1)
				{
					if (RendererRuntime::isInvalid(animationIndex))
					{
						throw std::runtime_error("The input file \"" + virtualInputFilename + "\" contains multiple animations, but the skeleton animation compiler wasn't provided with an animation index");
					}
				}
				else
				{
					// "When there's only one candidate, there's only one choice" (Monkey Island 1 quote)
					animationIndex = 0;
				}
				const aiAnimation* assimpAnimation = assimpScene->mAnimations[animationIndex];
				if (0 == assimpAnimation->mNumChannels)
				{
					throw std::runtime_error("The animation at index " + std::to_string(animationIndex) + " of input file \"" + virtualInputFilename + "\" has no channels");
				}

				// Determine whether or not bone scale is used, in case it's not ignored in general to start with
				// TODO(co) Optimization option: Currently, the automatic dynamic bone scale ignoring is over all animation channels. We could
				//          extend it that it's on per-channel base so that if one channel has bone scale while all other have not, only that
				//          one channel will save bone scale keys. We could do the same for position and rotation.
				if (!ignoreBoneScale)
				{
					// Let's be ignorant until someone proofs us wrong
					ignoreBoneScale = true;

					// Try to proof that the guy above is wrong and bone scale is used
					static const aiVector3D ONE_VECTOR(1.0f, 1.0f, 1.0f);
					for (unsigned int channel = 0; channel < assimpAnimation->mNumChannels && ignoreBoneScale; ++channel)
					{
						const aiNodeAnim* assimpNodeAnim = assimpAnimation->mChannels[channel];
						for (unsigned int i = 0; i < assimpNodeAnim->mNumScalingKeys; ++i)
						{
							if (!assimpNodeAnim->mScalingKeys[i].mValue.Equal(ONE_VECTOR, 1e-5f))
							{
								ignoreBoneScale = false;
								break;
							}
						}
					}
				}

				// Calculate the number of bytes required to store the complete animation data
				std::vector<uint32_t> channelByteOffsets;
				channelByteOffsets.resize(assimpAnimation->mNumChannels);
				uint32_t numberOfChannelDataBytes = 0;
				for (unsigned int channel = 0; channel < assimpAnimation->mNumChannels; ++channel)
				{
					const aiNodeAnim* assimpNodeAnim = assimpAnimation->mChannels[channel];
					channelByteOffsets[channel] = numberOfChannelDataBytes;
					numberOfChannelDataBytes += sizeof(RendererRuntime::SkeletonAnimationResource::ChannelHeader);
					numberOfChannelDataBytes += sizeof(RendererRuntime::SkeletonAnimationResource::Vector3Key) * assimpNodeAnim->mNumPositionKeys;
					numberOfChannelDataBytes += sizeof(RendererRuntime::SkeletonAnimationResource::QuaternionKey) * assimpNodeAnim->mNumRotationKeys;
					if (!ignoreBoneScale)
					{
						numberOfChannelDataBytes += sizeof(RendererRuntime::SkeletonAnimationResource::Vector3Key) * assimpNodeAnim->mNumScalingKeys;
					}
				}

				{ // Write down the skeleton animation header
					RendererRuntime::v1SkeletonAnimation::SkeletonAnimationHeader skeletonAnimationHeader;
					skeletonAnimationHeader.numberOfChannels		 = static_cast<uint8_t>(assimpAnimation->mNumChannels);
					skeletonAnimationHeader.durationInTicks			 = static_cast<float>(assimpAnimation->mDuration);
					skeletonAnimationHeader.ticksPerSecond			 = static_cast<float>(assimpAnimation->mTicksPerSecond);
					skeletonAnimationHeader.numberOfChannelDataBytes = numberOfChannelDataBytes;
					memoryFile.write(&skeletonAnimationHeader, sizeof(RendererRuntime::v1SkeletonAnimation::SkeletonAnimationHeader));
				}

				// Write down the channel byte offsets
				memoryFile.write(channelByteOffsets.data(), sizeof(uint32_t) * channelByteOffsets.size());

				// Bone channels, all the skeleton animation data in one big chunk
				for (unsigned int channel = 0; channel < assimpAnimation->mNumChannels; ++channel)
				{
					const aiNodeAnim* assimpNodeAnim = assimpAnimation->mChannels[channel];
					const uint32_t numScalingKeys = ignoreBoneScale ? 0 : assimpNodeAnim->mNumScalingKeys;

					{ // Bone channel header
						RendererRuntime::SkeletonAnimationResource::ChannelHeader channelHeader;
						channelHeader.boneId			   = RendererRuntime::StringId::calculateFNV(assimpNodeAnim->mNodeName.C_Str());
						channelHeader.numberOfPositionKeys = assimpNodeAnim->mNumPositionKeys;
						channelHeader.numberOfRotationKeys = assimpNodeAnim->mNumRotationKeys;
						channelHeader.numberOfScaleKeys	   = numScalingKeys;

						// Write down the bone channel header
						memoryFile.write(&channelHeader, sizeof(RendererRuntime::SkeletonAnimationResource::ChannelHeader));
					}

					// Write bone channel position data
					if (assimpNodeAnim->mNumPositionKeys > 0)
					{
						std::vector<RendererRuntime::SkeletonAnimationResource::Vector3Key> positionKeys;
						positionKeys.resize(assimpNodeAnim->mNumPositionKeys);
						for (unsigned int i = 0; i < assimpNodeAnim->mNumPositionKeys; ++i)
						{
							const aiVectorKey& assimpVectorKey = assimpNodeAnim->mPositionKeys[i];
							RendererRuntime::SkeletonAnimationResource::Vector3Key& vector3Key = positionKeys[i];
							vector3Key.timeInTicks = static_cast<float>(assimpVectorKey.mTime);
							vector3Key.value.x	   = assimpVectorKey.mValue.x;
							vector3Key.value.y	   = assimpVectorKey.mValue.y;
							vector3Key.value.z	   = assimpVectorKey.mValue.z;
						}
						memoryFile.write(positionKeys.data(), sizeof(RendererRuntime::SkeletonAnimationResource::Vector3Key) * assimpNodeAnim->mNumPositionKeys);
					}

					// Write bone channel rotation data
					// -> Some Assimp importers like the MD5 one compensate coordinate system differences by setting a root node transform, so we need to take this into account
					// -> We only store the xyz quaternion value of this key, w will be reconstructed during runtime
					if (assimpNodeAnim->mNumRotationKeys > 0)
					{
						std::vector<RendererRuntime::SkeletonAnimationResource::QuaternionKey> rotationKeys;
						rotationKeys.resize(assimpNodeAnim->mNumRotationKeys);
						const aiQuaternion assimpQuaternionOffset(aiMatrix3x3(assimpScene->mRootNode->mTransformation));
						const bool isMd5 = (aiString("<MD5_Hierarchy>") == assimpScene->mRootNode->mName);
						for (unsigned int i = 0; i < assimpNodeAnim->mNumRotationKeys; ++i)
						{
							const aiQuatKey& assimpQuatKey = assimpNodeAnim->mRotationKeys[i];
							RendererRuntime::SkeletonAnimationResource::QuaternionKey& quaternionKey = rotationKeys[i];
							aiQuaternion assimpQuaternion = (0 == channel) ? (assimpQuaternionOffset * assimpQuatKey.mValue) : assimpQuatKey.mValue;
							if (!isMd5)
							{
								// TODO(co) Somehow there's a flip when loading OGRE/MD5 skeleton animations. Haven't tried other formats, yet.
								assimpQuaternion.Conjugate();
							}
							quaternionKey.timeInTicks = static_cast<float>(assimpQuatKey.mTime);
							quaternionKey.value[0]	  = assimpQuaternion.x;
							quaternionKey.value[1]	  = assimpQuaternion.y;
							quaternionKey.value[2]	  = assimpQuaternion.z;
						}
						memoryFile.write(rotationKeys.data(), sizeof(RendererRuntime::SkeletonAnimationResource::QuaternionKey) * assimpNodeAnim->mNumRotationKeys);
					}

					// Write optional bone channel scale data
					if (numScalingKeys > 0)
					{
						std::vector<RendererRuntime::SkeletonAnimationResource::Vector3Key> scaleKeys;
						scaleKeys.resize(numScalingKeys);
						for (unsigned int i = 0; i < numScalingKeys; ++i)
						{
							const aiVectorKey& assimpVectorKey = assimpNodeAnim->mScalingKeys[i];
							RendererRuntime::SkeletonAnimationResource::Vector3Key& vector3Key = scaleKeys[i];
							vector3Key.timeInTicks = static_cast<float>(assimpVectorKey.mTime);
							vector3Key.value.x	   = assimpVectorKey.mValue.x;
							vector3Key.value.y	   = assimpVectorKey.mValue.y;
							vector3Key.value.z	   = assimpVectorKey.mValue.z;
						}
						memoryFile.write(scaleKeys.data(), sizeof(RendererRuntime::SkeletonAnimationResource::Vector3Key) * numScalingKeys);
					}
				}
			}
			else
			{
				throw std::runtime_error("Assimp failed to load in the given skeleton \"" + virtualInputFilename + "\": " + assimpLogStream.getLastErrorMessage());
			}

			// Write LZ4 compressed output
			memoryFile.writeLz4CompressedDataByVirtualFilename(RendererRuntime::v1SkeletonAnimation::FORMAT_TYPE, RendererRuntime::v1SkeletonAnimation::FORMAT_VERSION, input.context.getFileManager(), virtualOutputAssetFilename.c_str());

			// Store new cache entries or update existing ones
			input.cacheManager.storeOrUpdateCacheEntries(cacheEntries);
		}

		{ // Update the output asset package
			const std::string assetCategory = rapidJsonValueAsset["AssetMetadata"]["AssetCategory"].GetString();
			const std::string assetIdAsString = input.projectName + "/SkeletonAnimation/" + assetCategory + '/' + assetName;
			outputAsset(input.context.getFileManager(), assetIdAsString, virtualOutputAssetFilename, *output.outputAssetPackage);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
