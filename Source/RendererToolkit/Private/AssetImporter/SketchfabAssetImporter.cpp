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
#include "RendererToolkit/Private/AssetImporter/SketchfabAssetImporter.h"
#include "RendererToolkit/Private/Helper/AssimpLogStream.h"
#include "RendererToolkit/Private/Helper/AssimpIOSystem.h"
#include "RendererToolkit/Private/Helper/AssimpHelper.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Context.h"

#include <RendererRuntime/Public/Core/File/IFile.h>
#include <RendererRuntime/Public/Core/File/IFileManager.h>
#include <RendererRuntime/Public/Core/File/FileSystemHelper.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator 'FORCE_32BIT' in switch of enum 'aiMetadataType' is not explicitly handled by a case label
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'uint32_t' to 'int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::_Tree<std::_Tmap_traits<_Kty,_Ty,_Pr,_Alloc,false>>': copy constructor was implicitly defined as deleted
	#include <assimp/scene.h>
	#include <assimp/Importer.hpp>
	#include <../code/RemoveRedundantMaterials.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4061)	// warning C4061: enumerator 'rapidjson::kNumberType' in switch of enum 'rapidjson::Type' is not explicitly handled by a case label
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'int' to 'rapidjson::internal::BigInteger::Type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'rapidjson::GenericMember<Encoding,Allocator>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <rapidjson/document.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__BYTE_ORDER__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	// For ZIP-archive extraction we're using miniz which comes with Crunch
	#include <../src/crn_miniz.h>
PRAGMA_WARNING_POP

#include <array>
#include <unordered_map>


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
		typedef std::vector<uint8_t> FileData;
		typedef std::vector<std::string> TextureFilenames;
		typedef std::unordered_map<std::string, TextureFilenames> MaterialTextureFilenames;	// Key = material name
		typedef std::unordered_map<std::string, std::string> MaterialNameToAssetId;	// Key = source material name (e.g. "/Head"), value = imported material filename (e.g. "./Spino_Head.asset")
		struct ImporterContext final
		{
			std::string			  meshFilename;
			bool				  hasSkeleton			   = false;
			bool				  removeRedundantMaterials = true;
			MaterialNameToAssetId materialNameToAssetId;
		};

		// Sketchfab supported mesh formats: https://help.sketchfab.com/hc/en-us/articles/202508396-3D-File-Formats
		// -> List is from October 27'th, 2017
		typedef std::array<const char*, 52> SketchfabMeshFormats;
		SketchfabMeshFormats g_SketchfabMeshFormats = {
			".3dc", ".asc",															// 3DC point cloud
			".3ds",																	// 3DS
			".ac",																	// ac3d
			".abc",																	// Alembic
			".obj",																	// Alias Wavefront
			".bvh",																	// Biovision Hierarchy
			".blend",																// Blender
			".geo",																	// Carbon Graphics Inc
			".dae", ".zae",															// Collada
			".dwf",																	// Design Web Format
			".dw",																	// Designer Workbench
			".x",																	// DirectX
			".dxf",																	// Drawing eXchange Format
			".fbx",																	// Autodesk Filmbox, FBX
			".ogr",																	// GDAL vector format
			".gta",																	// Generic Tagged Arrays
			".gltf", ".glb",														// GL Transmission Format
			".igs", ".iges",														// Initial Graphics Exchange Specification, IGES
			".mu", ".craft",														// Kerbal Space Program
			".kmz",																	// Google Earth, Keyhole Markup Language
			".las",																	// LIDAR point clouds
			".lwo", ".lws",															// Lightwave
			".q3d",																	// Mimesys Q3D
			".mc2obj", ".dat",														// Minecraft
			".flt",																	// Open Flight
			".iv",																	// Open Inventor
			".osg", ".osgt", ".osgb", ".osgterrain", ".osgtgz", ".osgx", ".ive",	// OpenSceneGraph
			".ply",																	// Polygon File Format
			".bsp",																	// Quake
			".md2", ".mdl", 														// Quake / Valve source engine
			".shp",																	// Shape
			".stl", ".sta",															// Stereolithography, Standard Tessellation Language
			".txp",																	// Terrapage format database
			".vpk",																	// Valve source engine
			".wrl", ".vrml", ".wrz"													// Virtual Reality Modeling Language, VRML
		};

		// TODO(co) Add more supported mesh formats, but only add tested mesh formats so we know it's working in general
		typedef std::array<const char*, 4> SupportedMeshFormats;
		SupportedMeshFormats g_SupportedMeshFormats =
		{
			".obj",			// Alias Wavefront
			".fbx",			// Autodesk Filmbox, FBX
			".gltf", ".glb"	// GL Transmission Format
		};

		/*
		Sketchfab texture naming conventions: https://help.sketchfab.com/hc/en-us/articles/202600873-Materials-and-Textures#textures-auto-pbr
		"
		Automatic PBR Mapping

		Use our texture naming conventions to help us automatically choose settings and apply textures to the right shader slots. The format is "MaterialName_suffix.extension". For example, if you have a material named "Material1", you could name your textures like "Material1_diffuse.png", "Material1_metallic.png", etc.

		Avoid names with special characters, especially periods '.', underscores '_', and hyphens '-' because it can break the match.

		These are the strings we look for in the suffix:

		- Diffuse / Albedo / Base Color: 'diffuse', 'albedo', 'basecolor'
		- Metalness: 'metalness', 'metallic', 'metal', 'm'
		- Specular: 'specular', 'spec', 's'
		- Specular F0: 'specularf0', 'f0'
		- Roughness: 'roughness', 'rough', 'r'
		- Glossiness: 'glossiness', 'glossness', 'gloss', 'g', 'glossy'
		- AO: 'ambient occlusion', 'ao', 'occlusion', 'lightmap', 'diffuseintensity'
		- Cavity: 'cavity'
		- Normal Map: '''normal', 'nrm', 'normalmap'
		- Bump Map: 'bump', 'bumpmap', 'heightmap'
		- Emission: 'emission', 'emit', 'emissive'
		- Transparency: 'transparency', 'transparent', 'opacity', 'mask', 'alpha'
		"
		- Found also undocumented semantics in downloaded Sketchfab files:
			- "", "d", "diff", "dif" = Diffuse map
			- "n", "norm" = Normal map
			- "glow" = Emissive map
			- "light", "Ambient_Occlusion", "AmbientOccl" = Ambient occlusion map
			- Case variations, of course
		- PBR on Sketchfab: https://help.sketchfab.com/hc/en-us/articles/204429595-Materials-PBR-
		*/
		enum SemanticType
		{
			ALBEDO_MAP,
			NORMAL_MAP,
			HEIGHT_MAP,
			ROUGHNESS_MAP,
			GLOSS_MAP,
			METALLIC_MAP,
			EMISSIVE_MAP,
			NUMBER_OF_SEMANTICS
		};
		typedef std::vector<const char*> SemanticStrings;
		typedef std::array<SemanticStrings, SemanticType::NUMBER_OF_SEMANTICS> Semantics;
		const Semantics g_Semantics =
		{{
			// ALBEDO_MAP
			{ "diffuse", "albedo", "basecolor", "", "d", "diff", "dif" },
			// NORMAL_MAP
			{ "normal", "nrm", "normalmap", "n", "norm" },
			// HEIGHT_MAP
			{ "bump", "bumpmap", "heightmap" },
			// ROUGHNESS_MAP
			{ "roughness", "rough", "r" },
			// GLOSS_MAP
			{ "glossiness", "glossness", "gloss", "g", "glossy" },
			// METALLIC_MAP
			{ "metalness", "metallic", "metal", "m",
			  "specular", "spec", "s"		// Specular = roughness	TODO(co) Need a support strategy for specular map
			},
			// EMISSIVE_MAP
			{ "emission", "emit", "emissive", "glow" }
		}};


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void readFileIntoMemory(const RendererToolkit::IAssetImporter::Input& input, FileData& fileData)
		{
			RendererRuntime::IFileManager& fileManager = input.context.getFileManager();
			RendererRuntime::IFile* file = fileManager.openFile(RendererRuntime::IFileManager::FileMode::READ, input.absoluteSourceFilename.c_str());
			if (nullptr != file)
			{
				// Load the whole file content
				const size_t numberOfBytes = file->getNumberOfBytes();
				fileData.resize(numberOfBytes);
				file->read(const_cast<void*>(static_cast<const void*>(fileData.data())), numberOfBytes);	// TODO(co) Get rid of the evil const-cast

				// Close file
				fileManager.closeFile(*file);
			}
			else
			{
				// Error!
				throw std::runtime_error("Failed to open the Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + "\" for reading");
			}
		}

		void extractFromZipToFile(const RendererToolkit::IAssetImporter::Input& input, mz_zip_archive& zipArchive, mz_uint fileIndex, const char* filename)
		{
			// Ensure the directory exists
			RendererRuntime::IFileManager& fileManager = input.context.getFileManager();
			fileManager.createDirectories(input.virtualAssetOutputDirectory.c_str());

			// Write down the uncompressed file
			// -> Silently ignore and overwrite already existing files (might be a re-import)
			const std::string virtualFilename = input.virtualAssetOutputDirectory + '/' + std_filesystem::path(filename).filename().generic_string();
			RendererRuntime::IFile* file = fileManager.openFile(RendererRuntime::IFileManager::FileMode::WRITE, virtualFilename.c_str());
			if (nullptr != file)
			{
				// Write down the decompress file
				size_t uncompressedFileSize = 0;
				void* fileData = mz_zip_reader_extract_to_heap(&zipArchive, fileIndex, &uncompressedFileSize, 0);
				if (0 == uncompressedFileSize || nullptr == fileData)
				{
					// Error!
					throw std::runtime_error("Failed to extract the file \"" + std::string(filename) + "\" from Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + '\"');
				}
				file->write(fileData, uncompressedFileSize);
				mz_free(fileData);

				// Close file
				fileManager.closeFile(*file);
			}
			else
			{
				// Error!
				throw std::runtime_error("Failed to open the file \"" + virtualFilename + "\" for writing");
			}
		}

		void importTexture(const RendererToolkit::IAssetImporter::Input& input, mz_zip_archive& zipArchive, mz_uint fileIndex, const char* filename, TextureFilenames& textureFilenames)
		{
			// Ignore texture duplicates
			// -> Found such texture duplicates in several downloadable Sketchfab meshes, "Centaur" ( https://sketchfab.com/models/0d3f1b4a51144b7fbc4e2ff64d858413 )
			//    for example the has the same textures inside a "textures"-directory as well as inside "source\0c36ce708d3943b19c5a67da3cef9a81.zip"
			const std::string textureFilename = std_filesystem::path(filename).filename().generic_string();
			if (std::find(textureFilenames.cbegin(), textureFilenames.cend(), textureFilename) == textureFilenames.cend())
			{
				// Extract texture from ZIP-archive to file
				extractFromZipToFile(input, zipArchive, fileIndex, filename);

				// Remember the texture filename for creating the texture asset files later on
				textureFilenames.push_back(textureFilename);
			}
			else
			{
				RENDERER_LOG(input.context, WARNING, "The Sketchfab ZIP-archive \"%s\" contains multiple texture files named \"%s\", ignoring duplicates", input.absoluteSourceFilename.c_str(), textureFilename.c_str())
			}
		}

		void gatherMaterialTextureFilenames(const RendererToolkit::IAssetImporter::Input& input, const TextureFilenames& textureFilenames, MaterialTextureFilenames& materialTextureFilenames)
		{
			// Sanity check
			assert(!textureFilenames.empty());

			// Let's first see which materials and types of texture maps we have
			for (const std::string& textureFilename : textureFilenames)
			{
				// Get the texture filename stem: For example, the stem of "Spino_Head_N.tga.png" is "Spino_Head_N.tga"
				const std::string stem = std_filesystem::path(textureFilename).stem().generic_string();

				// Get the material name as well as the texture semantic
				// -> For example the material name of "Spino_Head_N.tga" is "Spino_Head"
				// -> For example the semantic of "Spino_Head_N.tga" is "N" which means normal map
				std::string materialName;
				std::string semanticAsString;
				const size_t lastSlashIndex = stem.find_last_of('_');
				if (std::string::npos != lastSlashIndex)
				{
					materialName = stem.substr(0, lastSlashIndex);
					const size_t dotIndex = stem.substr(lastSlashIndex + 1).find_first_of('.');
					semanticAsString = (std::string::npos != dotIndex) ? stem.substr(lastSlashIndex + 1, dotIndex) : stem.substr(lastSlashIndex + 1);
					RendererToolkit::StringHelper::toLowerCase(semanticAsString);
				}
				else
				{
					materialName = stem;
				}

				// Get per-material texture filename by semantics mapping
				TextureFilenames* textureFilenameBySemantics = nullptr;
				{
					MaterialTextureFilenames::iterator iterator = materialTextureFilenames.find(materialName);
					if (iterator == materialTextureFilenames.cend())
					{
						// New material discovered
						textureFilenameBySemantics = &materialTextureFilenames.emplace(materialName, TextureFilenames()).first->second;
						textureFilenameBySemantics->resize(SemanticType::NUMBER_OF_SEMANTICS);
					}
					else
					{
						// Material is already known
						textureFilenameBySemantics = &iterator->second;
					}
				}

				// Evaluate the texture semantic
				for (size_t semanticIndex = 0; semanticIndex < SemanticType::NUMBER_OF_SEMANTICS; ++semanticIndex)
				{
					const SemanticStrings& semanticStrings = g_Semantics[semanticIndex];
					if (std::find(semanticStrings.cbegin(), semanticStrings.cend(), semanticAsString) != semanticStrings.cend())
					{
						std::string& textureFilenameBySemantic = textureFilenameBySemantics->at(semanticIndex);
						if (!textureFilenameBySemantic.empty())
						{
							// Error!
							throw std::runtime_error("The Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + "\" contains multiple texture files like \"" + textureFilename + "\" with the same semantic for material \"" + materialName + '\"');
						}
						textureFilenameBySemantic = textureFilename;
						break;
					}
				}
			}
		}

		void createTextureChannelPackingAssetFile(const RendererToolkit::IAssetImporter::Input& input, const std::string& materialName, const TextureFilenames& textureFilenames, const std::string& semantic)
		{
			/* Example for a resulting texture asset JSON file
			{
				"Format": {
					"Type": "Asset",
					"Version": "1"
				},
				"Asset": {
					"Compiler": {
						"ClassName": "RendererToolkit::TextureAssetCompiler",
						"TextureSemantic": "PACKED_CHANNELS",
						"TextureChannelPacking": "_argb_nxa",
						"InputFiles": {
							"ALBEDO_MAP": "./Spino_Body_D.tga.png",
							"NORMAL_MAP": "./Spino_Body_N.tga.png"
						}
					}
				}
			}
			*/
			rapidjson::Document rapidJsonDocumentAsset(rapidjson::kObjectType);
			rapidjson::Document::AllocatorType& rapidJsonAllocatorType = rapidJsonDocumentAsset.GetAllocator();
			rapidjson::Value rapidJsonValueAsset(rapidjson::kObjectType);

			{ // Texture asset compiler
				rapidjson::Value rapidJsonValueCompiler(rapidjson::kObjectType);
				rapidJsonValueCompiler.AddMember("ClassName", "RendererToolkit::TextureAssetCompiler", rapidJsonAllocatorType);

				// Semantic dependent handling
				if ("_argb_nxa" == semantic || "_hr_rg_mb_nya" == semantic)
				{
					// Texture channel packing
					rapidJsonValueCompiler.AddMember("TextureSemantic", "PACKED_CHANNELS", rapidJsonAllocatorType);
					rapidJsonValueCompiler.AddMember("TextureChannelPacking", rapidjson::StringRef(semantic), rapidJsonAllocatorType);

					// Define helper macro
					#define ADD_MEMBER(semanticType) \
						if (!textureFilenames[SemanticType::semanticType].empty()) \
						{ \
							rapidJsonValueInputFiles.AddMember(#semanticType, "./" + textureFilenames[SemanticType::semanticType], rapidJsonAllocatorType); \
						}

					{ // Input files
						rapidjson::Value rapidJsonValueInputFiles(rapidjson::kObjectType);
						if ("_argb_nxa" == semantic)
						{
							ADD_MEMBER(ALBEDO_MAP)
							ADD_MEMBER(NORMAL_MAP)
						}
						else if ("_hr_rg_mb_nya" == semantic)
						{
							// Sanity check
							if (!textureFilenames[SemanticType::ROUGHNESS_MAP].empty() && !textureFilenames[SemanticType::GLOSS_MAP].empty())
							{
								// Error!
								throw std::runtime_error("Failed to import Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + "\" since material \"" + materialName + "\" is referencing a roughness map as well as a gloss map, but only one of those are allowed at one and the same time");
							}

							// Add members
							ADD_MEMBER(HEIGHT_MAP)
							ADD_MEMBER(ROUGHNESS_MAP)
							ADD_MEMBER(GLOSS_MAP)
							ADD_MEMBER(METALLIC_MAP)
							ADD_MEMBER(NORMAL_MAP)
						}
						else
						{
							// Error!
							assert(false && "Broken implementation, we should never ever be in here");
						}
						rapidJsonValueCompiler.AddMember("InputFiles", rapidJsonValueInputFiles, rapidJsonAllocatorType);
					}

					// Undefine helper macro
					#undef ADD_MEMBER
				}
				else if ("_e" == semantic)
				{
					// No texture channel packing
					rapidJsonValueCompiler.AddMember("TextureSemantic", "EMISSIVE_MAP", rapidJsonAllocatorType);
					rapidJsonValueCompiler.AddMember("InputFile", "./" + textureFilenames[SemanticType::EMISSIVE_MAP], rapidJsonAllocatorType);
				}
				else
				{
					// Error!
					assert(false && "Broken implementation, we should never ever be in here");
				}

				// Add texture asset compiler member
				rapidJsonValueAsset.AddMember("Compiler", rapidJsonValueCompiler, rapidJsonAllocatorType);
			}

			// Write down the texture asset JSON file
			// -> Silently ignore and overwrite already existing files (might be a re-import)
			const std::string virtualFilename = input.virtualAssetOutputDirectory + '/' + materialName + semantic + ".asset";
			RendererToolkit::JsonHelper::saveDocumentByFilename(input.context.getFileManager(), virtualFilename, "Asset", "1", rapidJsonValueAsset);
		}

		void createTextureChannelPackingAssetFiles(const RendererToolkit::IAssetImporter::Input& input, const MaterialTextureFilenames& materialTextureFilenames)
		{
			// Iterate through the materials
			for (const auto& pair : materialTextureFilenames)
			{
				const std::string& materialName = pair.first;
				const TextureFilenames& textureFilenames = pair.second;

				// Texture channel packing "_argb_nxa"
				if (!textureFilenames[SemanticType::ALBEDO_MAP].empty() || !textureFilenames[SemanticType::NORMAL_MAP].empty())
				{
					createTextureChannelPackingAssetFile(input, materialName, textureFilenames, "_argb_nxa");
				}

				// Texture channel packing "_hr_rg_mb_nya"
				if (!textureFilenames[SemanticType::HEIGHT_MAP].empty() || !textureFilenames[SemanticType::ROUGHNESS_MAP].empty() || !textureFilenames[SemanticType::GLOSS_MAP].empty() ||
					!textureFilenames[SemanticType::METALLIC_MAP].empty() || !textureFilenames[SemanticType::NORMAL_MAP].empty())
				{
					createTextureChannelPackingAssetFile(input, materialName, textureFilenames, "_hr_rg_mb_nya");
				}

				// Emissive map "_e"
				if (!textureFilenames[SemanticType::EMISSIVE_MAP].empty())
				{
					createTextureChannelPackingAssetFile(input, materialName, textureFilenames, "_e");
				}
			}
		}

		void createMaterialFile(const RendererToolkit::IAssetImporter::Input& input, const std::string& materialName, const TextureFilenames& textureFilenames, const ImporterContext& importerContext)
		{
			/* Example for a resulting material JSON file
			{
				"Format": {
					"Type": "MaterialAsset",
					"Version": "1"
				},
				"MaterialAsset": {
					"BaseMaterial": "${PROJECT_NAME}/Material/Base/Mesh.asset",
					"Properties": {
						"_argb_nxa": "./Texture/Spino_Body_argb_nxa.asset",
						"_hr_rg_mb_nya": "./Texture/Spino_Body_hr_rg_mb_nya.asset"
					}
				}
			}
			*/
			rapidjson::Document rapidJsonDocumentAsset(rapidjson::kObjectType);
			rapidjson::Document::AllocatorType& rapidJsonAllocatorType = rapidJsonDocumentAsset.GetAllocator();
			rapidjson::Value rapidJsonValueMaterialAsset(rapidjson::kObjectType);
			const std::string baseMaterial = importerContext.hasSkeleton ? "${PROJECT_NAME}/Blueprint/Mesh/M_SkinnedMesh.asset" : "${PROJECT_NAME}/Blueprint/Mesh/M_Mesh.asset";
			const std::string relativeFilename_argb_nxa = "./" + materialName + "_argb_nxa" + ".asset";
			const std::string relativeFilename_hr_rg_mb_nya = "./" + materialName + "_hr_rg_mb_nya" + ".asset";
			const std::string relativeFilenameEmissiveMap = "./" + materialName + "_e" + ".asset";

			// Base material
			rapidJsonValueMaterialAsset.AddMember("BaseMaterial", rapidjson::StringRef(baseMaterial), rapidJsonAllocatorType);

			{ // Properties
				rapidjson::Value rapidJsonValueProperties(rapidjson::kObjectType);

				// Texture channel packing "_argb_nxa"
				if (!textureFilenames[SemanticType::ALBEDO_MAP].empty() || !textureFilenames[SemanticType::NORMAL_MAP].empty())
				{
					rapidJsonValueProperties.AddMember("_argb_nxa", rapidjson::StringRef(relativeFilename_argb_nxa), rapidJsonAllocatorType);
				}

				// Texture channel packing "_hr_rg_mb_nya"
				if (!textureFilenames[SemanticType::HEIGHT_MAP].empty() || !textureFilenames[SemanticType::ROUGHNESS_MAP].empty() || !textureFilenames[SemanticType::GLOSS_MAP].empty() ||
					!textureFilenames[SemanticType::METALLIC_MAP].empty() || !textureFilenames[SemanticType::NORMAL_MAP].empty())
				{
					rapidJsonValueProperties.AddMember("_hr_rg_mb_nya", rapidjson::StringRef(relativeFilename_hr_rg_mb_nya), rapidJsonAllocatorType);
				}

				// Emissive map "_e"
				if (!textureFilenames[SemanticType::EMISSIVE_MAP].empty())
				{
					rapidJsonValueProperties.AddMember("UseEmissiveMap", "TRUE", rapidJsonAllocatorType);
					rapidJsonValueProperties.AddMember("EmissiveMap", rapidjson::StringRef(relativeFilenameEmissiveMap), rapidJsonAllocatorType);
				}

				// Add properties member
				rapidJsonValueMaterialAsset.AddMember("Properties", rapidJsonValueProperties, rapidJsonAllocatorType);
			}

			// Write down the material JSON file
			// -> Silently ignore and overwrite already existing files (might be a re-import)
			const std::string virtualFilename = input.virtualAssetOutputDirectory + '/' + materialName + ".material";
			RendererToolkit::JsonHelper::saveDocumentByFilename(input.context.getFileManager(), virtualFilename, "MaterialAsset", "1", rapidJsonValueMaterialAsset);
		}

		void createMaterialAssetFiles(const RendererToolkit::IAssetImporter::Input& input, const MaterialTextureFilenames& materialTextureFilenames, const ImporterContext& importerContext)
		{
			// Iterate through the materials
			for (const auto& pair : materialTextureFilenames)
			{
				createMaterialFile(input, pair.first, pair.second, importerContext);
			}
		}

		void importMeshMtl(const RendererToolkit::IAssetImporter::Input& input, mz_zip_archive& zipArchive, mz_uint fileIndex, const char* filename)
		{
			// Extract MTL-file of OBJ mesh format from ZIP-archive to file
			extractFromZipToFile(input, zipArchive, fileIndex, filename);
		}

		// Due to many artist asset variations, the material name to asset ID is a tricky and error prone mapping
		void createMaterialNameToAssetIdForMaterial(const RendererToolkit::IAssetImporter::Input& input, const MaterialTextureFilenames& materialTextureFilenames, const std::string& assimpMaterialName, MaterialNameToAssetId& materialNameToAssetId)
		{
			// First, maybe we're in luck and we have a nice and clean exact match
			MaterialTextureFilenames::const_iterator iterator = materialTextureFilenames.find(assimpMaterialName);
			if (iterator != materialTextureFilenames.cend())
			{
				// We have a nice and clean exact match
				materialNameToAssetId.emplace(assimpMaterialName, "./" + assimpMaterialName + ".asset");
			}
			else
			{
				// How unexpected, the downloaded Sketchfab mesh is violating the Sketchfab texture naming conventions: https://help.sketchfab.com/hc/en-us/articles/202600873-Materials-and-Textures#textures-auto-pbr
				std::string materialName = assimpMaterialName;

				// Remove odd characters found in at least one downloaded Sketchfab mesh
				materialName.erase(std::remove(materialName.begin(), materialName.end(), '/'), materialName.end());

				// Try to find a material which might match
				for (const auto& pair : materialTextureFilenames)
				{
					const std::string& currentMaterialName = pair.first;
					if (currentMaterialName.find(materialName) != std::string::npos)
					{
						materialNameToAssetId.emplace(assimpMaterialName, "./" + currentMaterialName + ".asset");
						return;
					}
				}

				// Add an empty entry so the user knowns which materials need to be assigned manually
				materialNameToAssetId.emplace(assimpMaterialName, "");
				RENDERER_LOG(input.context, WARNING, "The Sketchfab asset importer failed to automatically find a material name to asset ID mapping of mesh material \"%s\" from the Sketchfab ZIP-archive \"%s\"", assimpMaterialName.c_str(), input.absoluteSourceFilename.c_str())
			}
		}

		void createMaterialNameToAssetId(const RendererToolkit::IAssetImporter::Input& input, const MaterialTextureFilenames& materialTextureFilenames, ImporterContext& importerContext)
		{
			// Create an instance of the Assimp importer class
			RendererToolkit::AssimpLogStream assimpLogStream;
			Assimp::Importer assimpImporter;
			assimpImporter.SetIOHandler(new RendererToolkit::AssimpIOSystem(input.context.getFileManager()));

			// Load the given mesh so we can figure out which materials are referenced
			// -> Since we're only interesting in referenced materials, Assimp doesn't need to perform any additional mesh processing
			const std::string virtualFilename = input.virtualAssetOutputDirectory + '/' + importerContext.meshFilename;
			const aiScene* assimpScene = assimpImporter.ReadFile(virtualFilename.c_str(), 0);
			if (nullptr != assimpScene && nullptr != assimpScene->mRootNode)
			{
				for (unsigned int materialIndex = 0; materialIndex < assimpScene->mNumMaterials; ++materialIndex)
				{
					const aiMaterial* assimpMaterial = assimpScene->mMaterials[materialIndex];
					aiString assimpMaterialName;
					assimpMaterial->Get(AI_MATKEY_NAME, assimpMaterialName);
					if (assimpMaterialName.length > 0 && nullptr == strstr(assimpMaterialName.C_Str(), AI_DEFAULT_MATERIAL_NAME))
					{
						// Let the guesswork begin
						createMaterialNameToAssetIdForMaterial(input, materialTextureFilenames, assimpMaterialName.C_Str(), importerContext.materialNameToAssetId);
					}
				}

				// Does the mesh have a skeleton?
				importerContext.hasSkeleton = (RendererToolkit::AssimpHelper::getNumberOfBones(*assimpScene->mRootNode) > 0);

				// Check whether or not it looks dangerous to use "aiProcess_RemoveRedundantMaterials"
				// -> "Centaur" ( https://sketchfab.com/models/0d3f1b4a51144b7fbc4e2ff64d858413 ) for example has only identical dummy
				//    entries inside the MTL-OBJ-file and removing redundant materials results in some wrong assigned materials
				{
					const unsigned int previousNumMaterials = assimpScene->mNumMaterials;
					Assimp::RemoveRedundantMatsProcess().Execute(const_cast<aiScene*>(assimpScene));	// TODO(co) Get rid of the evil const-cast
					if (previousNumMaterials != assimpScene->mNumMaterials)
					{
						importerContext.removeRedundantMaterials = false;
					}
				}
			}
			else
			{
				// Error!
				throw std::runtime_error("Assimp failed to load the mesh \"" + virtualFilename + "\" from the Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + "\": " + assimpLogStream.getLastErrorMessage());
			}
		}

		/*
		Sketchfab merging conventions: https://help.sketchfab.com/hc/en-us/articles/201766675-Viewer-Performance
		"
		Materials
		- Identical materials are merged together.

		Geometries
		- Meshes that share the same material are merged together.
		- Geometries are not merged for animated objects or objects with transparency!
		"
		*/
		void createMeshAssetFile(const RendererToolkit::IAssetImporter::Input& input, const ImporterContext& importerContext)
		{
			/* Example for a resulting mesh asset JSON file
			{
				"Format": {
					"Type": "Asset",
					"Version": "1"
				},
				"Asset": {
					"Compiler": {
						"ClassName": "RendererToolkit::MeshAssetCompiler",
						"InputFile": "./SpinosaurusAeg.obj",
						"MaterialNameToAssetId": {
							"/Head": "./Spino_Head.asset",
							"/Body": "./Spino_Body.asset"
						}
					}
				}
			}
			*/
			rapidjson::Document rapidJsonDocumentAsset(rapidjson::kObjectType);
			rapidjson::Document::AllocatorType& rapidJsonAllocatorType = rapidJsonDocumentAsset.GetAllocator();
			rapidjson::Value rapidJsonValueAsset(rapidjson::kObjectType);

			{ // Mesh asset compiler
				rapidjson::Value rapidJsonValueCompiler(rapidjson::kObjectType);
				rapidJsonValueCompiler.AddMember("ClassName", "RendererToolkit::MeshAssetCompiler", rapidJsonAllocatorType);
				rapidJsonValueCompiler.AddMember("InputFile", "./" + importerContext.meshFilename, rapidJsonAllocatorType);

				// Check whether or not it looks dangerous to use "aiProcess_RemoveRedundantMaterials"
				// -> "Centaur" ( https://sketchfab.com/models/0d3f1b4a51144b7fbc4e2ff64d858413 ) for example has only identical dummy
				//    entries inside the MTL-OBJ-file and removing redundant materials results in some wrong assigned materials
				if (!importerContext.removeRedundantMaterials)
				{
					rapidJsonValueCompiler.AddMember("ImportFlags", "DEFAULT_FLAGS & ~REMOVE_REDUNDANT_MATERIALS", rapidJsonAllocatorType);
				}

				// Add material name to asset ID mapping
				if (!importerContext.materialNameToAssetId.empty())
				{
					rapidjson::Value rapidJsonValueMaterialNameToAssetId(rapidjson::kObjectType);
					for (const auto& pair : importerContext.materialNameToAssetId)
					{
						rapidJsonValueMaterialNameToAssetId.AddMember(rapidjson::StringRef(pair.first), rapidjson::StringRef(pair.second), rapidJsonAllocatorType);
					}
					rapidJsonValueCompiler.AddMember("MaterialNameToAssetId", rapidJsonValueMaterialNameToAssetId, rapidJsonAllocatorType);
				}

				// Add mesh asset compiler member
				rapidJsonValueAsset.AddMember("Compiler", rapidJsonValueCompiler, rapidJsonAllocatorType);
			}

			// Write down the mesh asset JSON file
			// -> Silently ignore and overwrite already existing files (might be a re-import)
			const std::string virtualFilename = input.virtualAssetOutputDirectory + '/' + std_filesystem::path(importerContext.meshFilename).stem().generic_string() + ".asset";
			RendererToolkit::JsonHelper::saveDocumentByFilename(input.context.getFileManager(), virtualFilename, "Asset", "1", rapidJsonValueAsset);
		}

		void importMesh(const RendererToolkit::IAssetImporter::Input& input, mz_zip_archive& zipArchive, mz_uint fileIndex, const char* filename, ImporterContext& importerContext)
		{
			// Sanity check
			if (!importerContext.meshFilename.empty())
			{
				// Error!
				throw std::runtime_error("Failed to import Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + "\" since it contains multiple mesh files");
			}

			// Extract mesh from ZIP-archive to file
			extractFromZipToFile(input, zipArchive, fileIndex, filename);
			importerContext.meshFilename = std_filesystem::path(filename).filename().generic_string();
		}

		void importByZipArchive(const RendererToolkit::IAssetImporter::Input& input, const FileData& fileData, ImporterContext& importerContext, TextureFilenames& textureFilenames)
		{
			// Initialize the ZIP-archive
			mz_zip_archive zipArchive = {};
			if (!mz_zip_reader_init_mem(&zipArchive, static_cast<const void*>(fileData.data()), fileData.size(), MZ_ZIP_FLAG_CASE_SENSITIVE | MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY))
			{
				// Error!
				throw std::runtime_error("Failed to initialize opened Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + "\" for reading");
			}

			// Iterate through the ZIP-archive files
			const mz_uint numberOfFiles = mz_zip_reader_get_num_files(&zipArchive);
			for (mz_uint fileIndex = 0; fileIndex < numberOfFiles; ++fileIndex)
			{
				// Get filename and file extension
				const mz_uint FILENAME_BUFFER_SIZE = 256;
				char filename[FILENAME_BUFFER_SIZE];
				const mz_uint filenameSize = mz_zip_reader_get_filename(&zipArchive, fileIndex, filename, FILENAME_BUFFER_SIZE);
				if (0 == filenameSize)
				{
					// Error!
					throw std::runtime_error("Failed to get filename at index " + std::to_string(fileIndex) + " while reading the Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + '\"');
				}
				std::string extension = std_filesystem::path(filename).extension().generic_string();
				RendererToolkit::StringHelper::toLowerCase(extension);

				// Evaluate the file extension and proceed accordantly
				// -> Silently ignore unknown files

				// Texture: Sketchfab supported texture formats: https://help.sketchfab.com/hc/en-us/articles/202600873-Materials-and-Textures#textures-file-formats
				// -> "Anything that is not .JPG or .PNG is converted to .PNG."
				if (".jpg" == extension || ".png" == extension)
				{
					importTexture(input, zipArchive, fileIndex, filename, textureFilenames);
				}

				// Mesh
				else if (".zip" == extension)
				{
					// Import by ZIP in ZIP-archive
					mz_zip_archive_file_stat zipArchiveFileStat = {};
					if (!mz_zip_reader_file_stat(&zipArchive, fileIndex, &zipArchiveFileStat))
					{
						// Error!
						throw std::runtime_error("Failed to get information about the Sketchfab ZIP-file \"" + std::string(filename) + "\" from Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + '\"');
					}
					FileData zipFileData(static_cast<size_t>(zipArchiveFileStat.m_uncomp_size));
					if (!mz_zip_reader_extract_to_mem(&zipArchive, fileIndex, zipFileData.data(), static_cast<size_t>(zipArchiveFileStat.m_uncomp_size), 0))
					{
						// Error!
						throw std::runtime_error("Failed to extract the Sketchfab ZIP-file \"" + std::string(filename) + "\" from Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + '\"');
					}
					importByZipArchive(input, zipFileData, importerContext, textureFilenames);
				}
				else if (std::find(g_SupportedMeshFormats.cbegin(), g_SupportedMeshFormats.cend(), extension) != g_SupportedMeshFormats.cend())
				{
					importMesh(input, zipArchive, fileIndex, filename, importerContext);
				}
				else if (".mtl" == extension)
				{
					importMeshMtl(input, zipArchive, fileIndex, filename);
				}
				else if (std::find(g_SketchfabMeshFormats.cbegin(), g_SketchfabMeshFormats.cend(), extension) != g_SketchfabMeshFormats.cend())
				{
					// Error!
					throw std::runtime_error("Failed to import mesh asset \"" + std::string(filename) + "\" while reading the Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + "\": Mesh format \"" + extension + "\" isn't supported");
				}
			}

			// End the ZIP-archive
			if (!mz_zip_reader_end(&zipArchive))
			{
				// Error!
				throw std::runtime_error("Failed to close the read Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + '\"');
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
	//[ Public virtual RendererToolkit::IAssetImporter methods ]
	//[-------------------------------------------------------]
	void SketchfabAssetImporter::import(const Input& input)
	{
		// Read the ZIP-archive file into memory
		::detail::FileData fileData;
		::detail::readFileIntoMemory(input, fileData);

		// Import by ZIP-archive
		::detail::ImporterContext importerContext;
		::detail::TextureFilenames textureFilenames;
		::detail::importByZipArchive(input, fileData, importerContext, textureFilenames);
		if (importerContext.meshFilename.empty())
		{
			throw std::runtime_error("Failed to find mesh inside Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + '\"');
		}

		// Create texture and material asset files
		if (!textureFilenames.empty())
		{
			// Gather as much information as possible
			::detail::MaterialTextureFilenames materialTextureFilenames;
			::detail::gatherMaterialTextureFilenames(input, textureFilenames, materialTextureFilenames);
			if (materialTextureFilenames.empty())
			{
				throw std::runtime_error("Failed to gather material texture filenames for Sketchfab ZIP-archive \"" + input.absoluteSourceFilename + '\"');
			}
			::detail::createMaterialNameToAssetId(input, materialTextureFilenames, importerContext);

			// TODO(co) Skeleton support is under construction
			importerContext.hasSkeleton = false;

			// Write asset files
			::detail::createTextureChannelPackingAssetFiles(input, materialTextureFilenames);
			::detail::createMaterialAssetFiles(input, materialTextureFilenames, importerContext);
			::detail::createMeshAssetFile(input, importerContext);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
