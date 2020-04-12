/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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
#include "RendererToolkit/Private/AssetCompiler/TextureAssetCompiler.h"
#include "RendererToolkit/Private/Helper/StringHelper.h"
#include "RendererToolkit/Private/Helper/CacheManager.h"
#include "RendererToolkit/Private/Helper/JsonHelper.h"
#include "RendererToolkit/Private/Context.h"

#include <Renderer/Public/Asset/AssetPackage.h>
#include <Renderer/Public/Core/File/MemoryFile.h>
#include <Renderer/Public/Core/File/IFileManager.h>
#include <Renderer/Public/Core/File/FileSystemHelper.h>
#include <Renderer/Public/Resource/Texture/Loader/CrnArrayFileFormat.h>
#include <Renderer/Public/Resource/Texture/Loader/Lz4DdsTextureResourceLoader.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/glm.hpp>
	#include <glm/gtc/constants.hpp>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4005)						// warning C4005: '_HAS_EXCEPTIONS': macro redefinition
	PRAGMA_WARNING_DISABLE_MSVC(4018)						// warning C4018: '<': signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4061)						// warning C4061: enumerator 'crnlib::PIXEL_FMT_INVALID' in switch of enum 'crnlib::pixel_format' is not explicitly handled by a case label
	PRAGMA_WARNING_DISABLE_MSVC(4242)						// warning C4242: '=': conversion from 'crnd::uint32' to 'T', possible loss of data
	PRAGMA_WARNING_DISABLE_MSVC(4244)						// warning C4244: '=': conversion from 'crnd::uint32' to 'T', possible loss of data
	PRAGMA_WARNING_DISABLE_MSVC(4302)						// warning C4302: 'type cast': truncation from 'crnd::uint8 *' to 'crnd::uint32'
	PRAGMA_WARNING_DISABLE_MSVC(4311)						// warning C4311: 'type cast': pointer truncation from 'crnd::uint8 *' to 'crnd::uint32'
	PRAGMA_WARNING_DISABLE_MSVC(4312)						// warning C4312: 'type cast': conversion from 'int' to 'unsigned char *' of greater size
	PRAGMA_WARNING_DISABLE_MSVC(4365)						// warning C4365: 'argument': conversion from 'long' to 'crnlib::uint', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)						// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4555)						// warning C4555: result of expression not used
	PRAGMA_WARNING_DISABLE_MSVC(4574)						// warning C4574: 'CRNLIB_RESAMPLER_DEBUG_OPS' is defined to be '0': did you mean to use '#if CRNLIB_RESAMPLER_DEBUG_OPS'?
	PRAGMA_WARNING_DISABLE_MSVC(4548)						// warning C4548: expression before comma has no effect; expected expression with side-effect
	PRAGMA_WARNING_DISABLE_MSVC(4625)						// warning C4625: 'crnlib::pack_etc1_block_context': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)						// warning C4626: 'crnlib::pack_etc1_block_context': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)						// warning C4668: 'CRNLIB_SUPPORT_ETC_A1' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(5026)						// warning C5026: 'crnlib::pack_etc1_block_context': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)						// warning C5027: 'crnlib::pack_etc1_block_context': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5204)						// warning C5204: 'crnlib::task_pool::executable_task': class has virtual functions, but its trivial destructor is not virtual; instances of objects derived from this class may not be destructed correctly
	PRAGMA_WARNING_DISABLE_CLANG("-Wunused-value")			// warning: expression result unused [-Wunused-value]
	PRAGMA_WARNING_DISABLE_CLANG("-Warray-bounds")			// warning: array index 1 is past the end of the array (which contains 1 element) [-Warray-bounds]
	PRAGMA_WARNING_DISABLE_GCC("-Wunused-value")			// warning: expression result unused [-Wunused-value]
	PRAGMA_WARNING_DISABLE_GCC("-Wunused-local-typedefs")	// warning: typedef ‘<x>’ locally defined but not used [-Wunused-value]
	#include <crnlib.h>
	#include <dds_defs.h>
	#include <../src/crn_texture_conversion.h>
	#include <../src/crn_command_line_params.h>
	#include <../src/crn_stb_image.cpp>
	#include <../src/crn_console.h>
	#undef e	// Get rid of "unrimp\external\renderertoolkit\rapidjson\include\rapidjson\internal\diyfp.h(45): warning C4003: not enough arguments for function-like macro invocation 'e'"
PRAGMA_WARNING_POP

#include <ies/ies_loader.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: '=': conversion from 'int' to 'rapidjson::internal::BigInteger::Type', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'rapidjson::GenericMember<Encoding,Allocator>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(4866)	// warning C4866: compiler may not enforce left-to-right evaluation order for call to 'rapidjson::GenericValue<rapidjson::UTF8<char>,rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> >::operator[]<rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> >'
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <rapidjson/document.h>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::atomic_flag': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::atomic_flag': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::atomic_flag': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::atomic_flag': move assignment operator was implicitly defined as deleted
	#include <atomic>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	// Create Toksvig specular anti-aliasing to reduce shimmering
	// -> Basing on "Specular Showdown in the Wild West" by Stephen Hill - http://blog.selfshadow.com/2011/07/22/specular-showdown/ - http://www.selfshadow.com/sandbox/toksvig.html
	namespace toksvig
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		// Fixed build in values by intent: Don't provide the artists with too many opportunities to introduce editing problems and break consistency
		static constexpr float POWER = 100.0f;	///< Power {label:"Glossiness", default:100, min:0, max:256, step:1}
		static constexpr float SIGMA = 0.5f;	///< Sigma {label:"Filter width", default:0.5, step:0.02}


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		[[nodiscard]] float gaussianWeight(const glm::vec2& offset)
		{
			const float v = 2.0f * SIGMA * SIGMA;
			return exp(-glm::dot(offset, offset) / v) / (glm::pi<float>() * v);
		}

		[[nodiscard]] glm::vec4 fetch(crnlib::image_u8& normalMapCrunchImage, const glm::vec2& position, const glm::vec2& offset)
		{
			const crnlib::color_quad_u8& crunchColor = normalMapCrunchImage.get_clamped(static_cast<int>(position.x + offset.x), static_cast<int>(position.y + offset.y));
			const glm::vec3 n((crunchColor.r / 255.0f) * 2.0f - 1.0f, (crunchColor.g / 255.0f) * 2.0f - 1.0f, (crunchColor.b / 255.0f) * 2.0f - 1.0f);
			return glm::vec4(glm::normalize(n), 1.0f) * gaussianWeight(offset);
		}

		[[nodiscard]] float calculateToksvig(crnlib::image_u8& normalMapCrunchImage, const glm::vec2& position, float power)
		{
			// 3x3 filter
			glm::vec4 n = fetch(normalMapCrunchImage, position, glm::vec2(-1.0f, -1.0f));
			n += fetch(normalMapCrunchImage, position, glm::vec2( 0.0f, -1.0f));
			n += fetch(normalMapCrunchImage, position, glm::vec2( 1.0f, -1.0f));

			n += fetch(normalMapCrunchImage, position, glm::vec2(-1.0f,  0.0f));
			n += fetch(normalMapCrunchImage, position, glm::vec2( 0.0f,  0.0f));
			n += fetch(normalMapCrunchImage, position, glm::vec2( 1.0f,  0.0f));

			n += fetch(normalMapCrunchImage, position, glm::vec2(-1.0f,  1.0f));
			n += fetch(normalMapCrunchImage, position, glm::vec2( 0.0f,  1.0f));
			n += fetch(normalMapCrunchImage, position, glm::vec2( 1.0f,  1.0f));

			// Divide by weight sum
			n.x /= n.w;
			n.y /= n.w;
			n.z /= n.w;

			// Toksvig factor
			const float length = glm::length(glm::vec3(n));
			return length / glm::mix(power, 1.0f, length);
		}

		void createToksvigRoughnessMap(const crnlib::mip_level& normalMapCrunchMipLevel, crnlib::mip_level& toksvigCrunchMipLevel)
		{
			const crnlib::uint width = normalMapCrunchMipLevel.get_width();
			const crnlib::uint height = normalMapCrunchMipLevel.get_height();
			crnlib::image_u8* normalMapCrunchImage = normalMapCrunchMipLevel.get_image();
			crnlib::image_u8* crunchImage = toksvigCrunchMipLevel.get_image();
			for (crnlib::uint y = 0; y < height; ++y)
			{
				for (crnlib::uint x = 0; x < width; ++x)
				{
					// Toksvig: Areas in the original normal map that were flat are white (glossy), whereas noisy, bumpy sections are darker
					const float toksvig = glm::clamp(calculateToksvig(*normalMapCrunchImage, glm::vec2(x, y), POWER), 0.0f, 1.0f);

					// Roughness = 1 - glossiness
					(*crunchImage)(x, y) = static_cast<crnlib::uint8>((1.0f - toksvig) * 255.0f);
				}
			}
		}

		void compositeToksvigRoughnessMap(const crnlib::mip_level& roughnessMapCrunchMipLevel, const crnlib::mip_level& normalMapCrunchMipLevel, crnlib::mip_level& crunchMipLevel)
		{
			const crnlib::uint width = normalMapCrunchMipLevel.get_width();
			const crnlib::uint height = normalMapCrunchMipLevel.get_height();
			crnlib::image_u8* roughnessMapCrunchImage = roughnessMapCrunchMipLevel.get_image();
			crnlib::image_u8* normalMapCrunchImage = normalMapCrunchMipLevel.get_image();
			crnlib::image_u8* crunchImage = crunchMipLevel.get_image();
			for (crnlib::uint y = 0; y < height; ++y)
			{
				for (crnlib::uint x = 0; x < width; ++x)
				{
					// Toksvig: Areas in the original normal map that were flat are white (glossy), whereas noisy, bumpy sections are darker
					const float toksvig = glm::clamp(calculateToksvig(*normalMapCrunchImage, glm::vec2(x, y), POWER), 0.0f, 1.0f);

					// Roughness = 1 - glossiness
					const float originalGlossiness = 1.0f - ((*roughnessMapCrunchImage)(x, y).r / 255.0f);
					(*crunchImage)(x, y).r = 255u - static_cast<crnlib::uint8>(originalGlossiness * toksvig * 255.0f);
				}
			}
		}


	} // toksvig

	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Texture asset compiler texture semantic; used to automatically set semantic appropriate texture processing settings like "cCRNCompFlagPerceptual"-flags
		*/
		enum class TextureSemantic
		{
			ALBEDO_MAP,
			ALPHA_MAP,
			NORMAL_MAP,
			ROUGHNESS_MAP,					///< Roughness map ('_r'-postfix, aka specular F0, roughness = 1 - glossiness (= smoothness))
			GLOSS_MAP,						///< Gloss map (glossiness = 1 - roughness), during runtime only roughness map should be used hence gloss map is automatically converted into reflection map so artists don't need to manipulate texture source assets
			METALLIC_MAP,
			EMISSIVE_MAP,
			HEIGHT_MAP,
			TERRAIN_HEIGHT_MAP,				///< 16-bit height map
			TINT_MAP,
			AMBIENT_OCCLUSION_MAP,
			REFLECTION_2D_MAP,
			REFLECTION_CUBE_MAP,
			COLOR_CORRECTION_LOOKUP_TABLE,	///< Lookup table (LUT)
			PACKED_CHANNELS,
			VOLUME,							///< 3D volume data
			IES_LIGHT_PROFILE_ARRAY,		///< Illuminating Engineering Society (IES) light profile (photometric light data, use e.g. IESviewer ( http://photometricviewer.com/ ) as viewer)
			CRN_ARRAY,
			UNKNOWN
		};

		static constexpr uint16_t TEXTURE_FORMAT_VERSION = 0;

		typedef std::vector<std::string> Filenames;

		struct DdsHeaderDX10 final
		{
			uint32_t DXGIFormat; // See http://msdn.microsoft.com/en-us/library/bb173059.aspx
			uint32_t resourceDimension;
			uint32_t miscFlag;
			uint32_t arraySize;
			uint32_t reserved;
		};


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		static std::atomic<bool> g_CrunchInitialized = false;


		//[-------------------------------------------------------]
		//[ Global classes                                        ]
		//[-------------------------------------------------------]
		class MemoryStream final : public crnlib::data_stream
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			MemoryStream() :
				crnlib::data_stream("MemoryStream", crnlib::cDataStreamWritable),
				mFileSize(0),
				mOffset(0)
			{
				m_opened = true;
			}

			inline virtual ~MemoryStream() override
			{
				close();
			}

			[[nodiscard]] inline Renderer::MemoryFile& getMemoryFile()
			{
				return mMemoryFile;
			}


		//[-------------------------------------------------------]
		//[ Public virtual crnlib::data_stream methods            ]
		//[-------------------------------------------------------]
		public:
			virtual bool close() override
			{
				// Reset data
				mMemoryFile.getByteVector().clear();
				mFileSize = 0;
				mOffset	  = 0;
				m_opened  = false;
				m_error   = false;
				m_got_cr  = false;

				// Done
				return true;
			}

			[[nodiscard]] virtual crnlib::uint read(void*, crnlib::uint) override
			{
				// Write only
				CRNLIB_ASSERT(false);
				return 0;
			}

			[[nodiscard]] virtual crnlib::uint write(const void* pBuf, crnlib::uint len) override
			{
				CRNLIB_ASSERT(pBuf && (len <= 0x7FFFFFFF));
				if (!m_opened || !is_writable() || !len)
				{
					return 0;
				}
				mMemoryFile.write(pBuf, len);
				mOffset += len;
				mFileSize = crnlib::math::maximum(mFileSize, mOffset);
				return len;
			}

			inline virtual bool flush() override
			{
				// Nothing here
				return true;
			}

			[[nodiscard]] inline virtual crnlib::uint64 get_size() override
			{
				return m_opened ? mFileSize : 0;
			}

			[[nodiscard]] virtual crnlib::uint64 get_remaining() override
			{
				if (!m_opened)
				{
					return 0;
				}
				CRNLIB_ASSERT(mOffset <= mFileSize);
				return mFileSize - mOffset;
			}

			[[nodiscard]] inline virtual crnlib::uint64 get_ofs() override
			{
				return m_opened ? mOffset : 0;
			}

			inline virtual bool seek(crnlib::int64, bool) override
			{
				// Nothing here
				return false;
			}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			explicit MemoryStream(const MemoryStream& fileStream) = delete;
			MemoryStream& operator =(const MemoryStream& fileStream) = delete;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Renderer::MemoryFile mMemoryFile;
			uint64_t			 mFileSize;
			uint64_t			 mOffset;


		};

		class FileStream final : public crnlib::data_stream
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			FileStream(Renderer::IFileManager& fileManager, Renderer::IFileManager::FileMode fileMode, Renderer::VirtualFilename virtualFilename) :
				crnlib::data_stream(virtualFilename, crnlib::cDataStreamReadable),
				mFileManager(fileManager),
				mFile(fileManager.openFile(fileMode, virtualFilename)),
				mFileSize((nullptr != mFile) ? mFile->getNumberOfBytes() : 0),
				mOffset(0)
			{
				m_opened = (nullptr != mFile);
			}

			inline virtual ~FileStream() override
			{
				close();
			}


		//[-------------------------------------------------------]
		//[ Public virtual crnlib::data_stream methods            ]
		//[-------------------------------------------------------]
		public:
			virtual bool close() override
			{
				if (nullptr != mFile)
				{
					// Close file
					mFileManager.closeFile(*mFile);

					// Reset data
					mFile	  = nullptr;
					mFileSize = 0;
					mOffset	  = 0;
					m_opened  = false;
					m_error   = false;
					m_got_cr  = false;

					// Done
					return true;
				}

				// Error!
				return false;
			}

			[[nodiscard]] virtual crnlib::uint read(void* pBuf, crnlib::uint len) override
			{
				CRNLIB_ASSERT(pBuf && (len <= 0x7FFFFFFF));
				if (!m_opened || !is_readable() || !len)
				{
					return 0;
				}
				len = static_cast<crnlib::uint>(crnlib::math::minimum<crnlib::uint64>(len, get_remaining()));
				mFile->read(pBuf, len);
				mOffset += len;
				return len;
			}

			[[nodiscard]] virtual crnlib::uint write(const void* pBuf, crnlib::uint len) override
			{
				CRNLIB_ASSERT(pBuf && (len <= 0x7FFFFFFF));
				if (!m_opened || !is_writable() || !len)
				{
					return 0;
				}
				mFile->write(pBuf, len);
				mOffset += len;
				mFileSize = crnlib::math::maximum(mFileSize, mOffset);
				return len;
			}

			inline virtual bool flush() override
			{
				// Nothing here
				return true;
			}

			[[nodiscard]] inline virtual crnlib::uint64 get_size() override
			{
				return m_opened ? mFileSize : 0;
			}

			[[nodiscard]] virtual crnlib::uint64 get_remaining() override
			{
				if (!m_opened)
				{
					return 0;
				}
				CRNLIB_ASSERT(mOffset <= mFileSize);
				return mFileSize - mOffset;
			}

			[[nodiscard]] inline virtual crnlib::uint64 get_ofs() override
			{
				return m_opened ? mOffset : 0;
			}

			inline virtual bool seek(crnlib::int64, bool) override
			{
				// Nothing here
				return false;
			}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			explicit FileStream(const FileStream& fileStream) = delete;
			FileStream& operator =(const FileStream& fileStream) = delete;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Renderer::IFileManager& mFileManager;
			Renderer::IFile*		mFile;
			uint64_t				mFileSize;
			uint64_t				mOffset;


		};

		class FileDataStreamSerializer final : public crnlib::data_stream_serializer
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			FileDataStreamSerializer(Renderer::IFileManager& fileManager, Renderer::IFileManager::FileMode fileMode, Renderer::VirtualFilename virtualFilename) :
				mFileStream(fileManager, fileMode, virtualFilename)
			{
				if (!mFileStream.is_opened())
				{
					throw std::runtime_error("Failed to open source file \"" + std::string(virtualFilename) + '\"');
				}
				set_stream(&mFileStream);
			}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			explicit FileDataStreamSerializer(const FileDataStreamSerializer& fileDataStreamSerializer) = delete;
			FileDataStreamSerializer& operator =(const FileDataStreamSerializer& fileDataStreamSerializer) = delete;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			FileStream mFileStream;


		};


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void getVirtualOutputAssetFilenameAndCrunchOutputTextureFileType(const RendererToolkit::IAssetCompiler::Configuration& configuration, const std::string& assetFileFormat, const std::string& assetName, const std::string& virtualAssetOutputDirectory, std::string& virtualOutputAssetFilename, crnlib::texture_file_types::format& crunchOutputTextureFileType)
		{
			const rapidjson::Value& rapidJsonValueTargets = configuration.rapidJsonValueTargets;

			// Get the JSON targets object
			std::string textureTargetName;
			{
				const rapidjson::Value& rapidJsonValueRhiTargets = rapidJsonValueTargets["RhiTargets"];
				const rapidjson::Value& rapidJsonValueRhiTarget = rapidJsonValueRhiTargets[configuration.rhiTarget];
				textureTargetName = rapidJsonValueRhiTarget["TextureTarget"].GetString();
			}
			{
				std::string fileFormat = assetFileFormat;
				if (fileFormat.empty())
				{
					const rapidjson::Value& rapidJsonValueTextureTargets = rapidJsonValueTargets["TextureTargets"];
					const rapidjson::Value& rapidJsonValueTextureTarget = rapidJsonValueTextureTargets[textureTargetName];
					fileFormat = rapidJsonValueTextureTarget["FileFormat"].GetString();
				}
				virtualOutputAssetFilename = virtualAssetOutputDirectory + '/' + assetName + '.' + fileFormat;
				if (fileFormat == "crn")
				{
					crunchOutputTextureFileType = crnlib::texture_file_types::cFormatCRN;
				}
				else if (fileFormat == "lz4dds")
				{
					crunchOutputTextureFileType = crnlib::texture_file_types::cFormatDDS;
				}
				else if (fileFormat == "ktx")
				{
					crunchOutputTextureFileType = crnlib::texture_file_types::cFormatKTX;
				}
			}
		}

		[[nodiscard]] TextureSemantic getTextureSemanticByRapidJsonValue(const rapidjson::Value& rapidJsonValue)
		{
			const char* valueAsString = rapidJsonValue.GetString();
			const rapidjson::SizeType valueStringLength = rapidJsonValue.GetStringLength();

			// Define helper macros
			#define IF_VALUE(name)			 if (strncmp(valueAsString, #name, valueStringLength) == 0) return TextureSemantic::name;
			#define ELSE_IF_VALUE(name) else if (strncmp(valueAsString, #name, valueStringLength) == 0) return TextureSemantic::name;

			// Evaluate value
			IF_VALUE(ALBEDO_MAP)
			ELSE_IF_VALUE(ALPHA_MAP)
			ELSE_IF_VALUE(NORMAL_MAP)
			ELSE_IF_VALUE(ROUGHNESS_MAP)
			ELSE_IF_VALUE(GLOSS_MAP)
			ELSE_IF_VALUE(METALLIC_MAP)
			ELSE_IF_VALUE(EMISSIVE_MAP)
			ELSE_IF_VALUE(HEIGHT_MAP)
			ELSE_IF_VALUE(TERRAIN_HEIGHT_MAP)
			ELSE_IF_VALUE(TINT_MAP)
			ELSE_IF_VALUE(AMBIENT_OCCLUSION_MAP)
			ELSE_IF_VALUE(REFLECTION_2D_MAP)
			ELSE_IF_VALUE(REFLECTION_CUBE_MAP)
			ELSE_IF_VALUE(COLOR_CORRECTION_LOOKUP_TABLE)
			ELSE_IF_VALUE(PACKED_CHANNELS)
			ELSE_IF_VALUE(VOLUME)
			ELSE_IF_VALUE(IES_LIGHT_PROFILE_ARRAY)
			ELSE_IF_VALUE(CRN_ARRAY)
			else
			{
				throw std::runtime_error(std::string("Unknown texture semantic \"") + valueAsString + '\"');
			}

			// Undefine helper macros
			#undef IF_VALUE
			#undef ELSE_IF_VALUE
		}

		inline void mandatoryTextureSemanticProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, TextureSemantic& value)
		{
			value = getTextureSemanticByRapidJsonValue(rapidJsonValue[propertyName]);
		}

		void load2DCrunchMipmappedTextureInternal(Renderer::IFileManager& fileManager, Renderer::VirtualFilename virtualSourceFilename, crnlib::mipmapped_texture& crunchMipmappedTexture)
		{
			crnlib::texture_file_types::format crunchSourceFileFormat = crnlib::texture_file_types::determine_file_format(virtualSourceFilename);
			if (crunchSourceFileFormat == crnlib::texture_file_types::cFormatInvalid)
			{
				throw std::runtime_error("Unrecognized file type \"" + std::string(virtualSourceFilename) + '\"');
			}
			FileDataStreamSerializer fileDataStreamSerializer(fileManager, Renderer::IFileManager::FileMode::READ, virtualSourceFilename);
			if (!crunchMipmappedTexture.read_from_stream(fileDataStreamSerializer, crunchSourceFileFormat))
			{
				if (crunchMipmappedTexture.get_last_error().is_empty())
				{
					throw std::runtime_error("Failed reading source file \"" + std::string(virtualSourceFilename) + '\"');
				}
				else
				{
					throw std::runtime_error(crunchMipmappedTexture.get_last_error().get_ptr());
				}
			}
		}

		void load2DCrunchMipmappedTexture(Renderer::IFileManager& fileManager, Renderer::VirtualFilename virtualSourceFilename, Renderer::VirtualFilename virtualSourceNormalMapFilename, crnlib::mipmapped_texture& crunchMipmappedTexture, crnlib::texture_conversion::convert_params& crunchConvertParams)
		{
			// Load, generate or compose mipmapped Crunch texture
			if (nullptr != virtualSourceFilename && nullptr == virtualSourceNormalMapFilename)
			{
				// Just load source texture
				load2DCrunchMipmappedTextureInternal(fileManager, virtualSourceFilename, crunchMipmappedTexture);

				// Use source texture mipmaps?
				if (crnlib::texture_file_types::supports_mipmaps(crnlib::texture_file_types::determine_file_format(virtualSourceFilename)))
				{
					crunchConvertParams.m_mipmap_params.m_mode = cCRNMipModeUseSourceMips;
				}
			}
			else if (nullptr == virtualSourceFilename && nullptr != virtualSourceNormalMapFilename)
			{
				// Just generate a roughness map using a given normal map using Toksvig specular anti-aliasing to reduce shimmering

				// Load normal map texture
				crnlib::mipmapped_texture normalMapCrunchMipmappedTexture;
				load2DCrunchMipmappedTextureInternal(fileManager, virtualSourceNormalMapFilename, normalMapCrunchMipmappedTexture);

				// Create Toksvig specular anti-aliasing to reduce shimmering
				crunchMipmappedTexture.init(normalMapCrunchMipmappedTexture.get_width(), normalMapCrunchMipmappedTexture.get_height(), 1, 1, crnlib::PIXEL_FMT_L8, "Toksvig", crnlib::cDefaultOrientationFlags);
				::toksvig::createToksvigRoughnessMap(*normalMapCrunchMipmappedTexture.get_level(0, 0), *crunchMipmappedTexture.get_level(0, 0));
			}
			else
			{
				// Compose mipmapped Crunch texture

				// Load roughness map
				crnlib::mipmapped_texture roughnessMapCrunchMipmappedTexture;
				load2DCrunchMipmappedTextureInternal(fileManager, virtualSourceFilename, roughnessMapCrunchMipmappedTexture);

				// Load normal map
				crnlib::mipmapped_texture normalMapCrunchMipmappedTexture;
				load2DCrunchMipmappedTextureInternal(fileManager, virtualSourceNormalMapFilename, normalMapCrunchMipmappedTexture);

				// Sanity check
				if (roughnessMapCrunchMipmappedTexture.get_width() != normalMapCrunchMipmappedTexture.get_width() ||
					roughnessMapCrunchMipmappedTexture.get_height() != normalMapCrunchMipmappedTexture.get_height())
				{
					throw std::runtime_error("Roughness map and normal map must have the same dimension");
				}

				// Create Toksvig specular anti-aliasing to reduce shimmering
				crunchMipmappedTexture.init(normalMapCrunchMipmappedTexture.get_width(), normalMapCrunchMipmappedTexture.get_height(), 1, 1, crnlib::PIXEL_FMT_L8, "Toksvig", crnlib::cDefaultOrientationFlags);
				::toksvig::compositeToksvigRoughnessMap(*roughnessMapCrunchMipmappedTexture.get_level(0, 0), *normalMapCrunchMipmappedTexture.get_level(0, 0), *crunchMipmappedTexture.get_level(0, 0));
			}
		}

		[[nodiscard]] bool isToksvigSpecularAntiAliasingEnabled(const rapidjson::Value& rapidJsonValueTextureAssetCompiler)
		{
			bool toksvigSpecularAntiAliasing = false;
			RendererToolkit::JsonHelper::optionalBooleanProperty(rapidJsonValueTextureAssetCompiler, "ToksvigSpecularAntiAliasing", toksvigSpecularAntiAliasing);
			return toksvigSpecularAntiAliasing;
		}


		//[-------------------------------------------------------]
		//[ Global classes                                        ]
		//[-------------------------------------------------------]
		class TextureChannelPacking final
		{


		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		public:
			struct Source final
			{
				TextureSemantic			  textureSemantic  = TextureSemantic::UNKNOWN;
				uint8_t					  numberOfChannels = Renderer::getInvalid<uint8_t>();
				float					  defaultColor[4]  = { 0.0f, 0.0f, 0.0f, 0.0f };
				crnlib::mipmapped_texture crunchMipmappedTexture;
			};
			typedef std::vector<Source> Sources;

			struct Destination final
			{
				uint8_t sourceIndex	  = Renderer::getInvalid<uint8_t>();
				uint8_t sourceChannel = Renderer::getInvalid<uint8_t>();
			};
			typedef std::vector<Destination> Destinations;


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			TextureChannelPacking(Renderer::IFileManager& fileManager, const RendererToolkit::IAssetCompiler::Configuration& configuration, const rapidjson::Value& rapidJsonValueTextureAssetCompiler, const char* basePath, Renderer::VirtualFilename virtualSourceNormalMapFilename, crnlib::texture_conversion::convert_params& crunchConvertParams)
			{
				loadLayout(configuration, rapidJsonValueTextureAssetCompiler, crunchConvertParams);
				loadSourceCrunchMipmappedTextures(fileManager, rapidJsonValueTextureAssetCompiler, basePath, virtualSourceNormalMapFilename);
			}

			[[nodiscard]] crnlib::uint getDestinationWidth() const
			{
				for (const Source& source : mSources)
				{
					if (source.crunchMipmappedTexture.is_valid())
					{
						return source.crunchMipmappedTexture.get_width();
					}
				}
				throw std::runtime_error("Texture channel packing needs at least one source texture");
			}

			[[nodiscard]] crnlib::uint getDestinationHeight() const
			{
				for (const Source& source : mSources)
				{
					if (source.crunchMipmappedTexture.is_valid())
					{
						return source.crunchMipmappedTexture.get_height();
					}
				}
				throw std::runtime_error("Texture channel packing needs at least one source texture");
			}

			[[nodiscard]] crnlib::pixel_format getDestinationCrunchPixelFormat() const
			{
				switch (mDestinations.size())
				{
					case 1:
						return crnlib::PIXEL_FMT_L8;

					case 3:
						return crnlib::PIXEL_FMT_R8G8B8;

					case 4:
						return crnlib::PIXEL_FMT_A8R8G8B8;

					default:
						throw std::runtime_error("Invalid number of destination channels, must be 1, 3 or 4");
				}
			}

			[[nodiscard]] inline const Sources& getSources() const
			{
				return mSources;
			}

			[[nodiscard]] inline const Destinations& getDestinations() const
			{
				return mDestinations;
			}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			void loadLayout(const RendererToolkit::IAssetCompiler::Configuration& configuration, const rapidjson::Value& rapidJsonValueTextureAssetCompiler, crnlib::texture_conversion::convert_params& crunchConvertParams)
			{
				const std::string textureChannelPacking = rapidJsonValueTextureAssetCompiler["TextureChannelPacking"].GetString();
				const rapidjson::Value& rapidJsonValueTextureChannelPackings = configuration.rapidJsonValueTargets["TextureChannelPackings"];
				const rapidjson::Value& rapidJsonValueTextureChannelPacking = rapidJsonValueTextureChannelPackings[textureChannelPacking];

				{ // Sources
					const rapidjson::Value& rapidJsonValueSources = rapidJsonValueTextureChannelPacking["Sources"];
					if (rapidJsonValueSources.Size() > 4)
					{
						throw std::runtime_error("Texture channel packing \"" + textureChannelPacking + "\" has more than four sources which is invalid");
					}
					mSources.resize(rapidJsonValueSources.Size());
					for (rapidjson::SizeType i = 0; i < rapidJsonValueSources.Size(); ++i)
					{
						const rapidjson::Value& rapidJsonValueSource = rapidJsonValueSources[i];
						Source& source = mSources[i];
						mandatoryTextureSemanticProperty(rapidJsonValueSource, "TextureSemantic", source.textureSemantic);
						for (rapidjson::size_t k = 0; k < i; ++k)
						{
							if (mSources[k].textureSemantic == source.textureSemantic)
							{
								throw std::runtime_error("Texture channel packing \"" + textureChannelPacking + "\" source " + std::to_string(i) + ": The texture semantic \"" + rapidJsonValueSource["TextureSemantic"].GetString() + "\" is used multiple times which is invalid");
							}
						}
						source.numberOfChannels = static_cast<uint8_t>(rapidJsonValueSource["NumberOfChannels"].GetUint());
						if (source.numberOfChannels > 4)
						{
							throw std::runtime_error("Texture channel packing \"" + textureChannelPacking + "\" source " + std::to_string(i) + ": The number of texture channel packing source channels must not be greater as four");
						}
						RendererToolkit::JsonHelper::optionalFloatNProperty(rapidJsonValueSource, "DefaultColor", source.defaultColor, source.numberOfChannels);
					}
				}

				{ // Destinations
					const rapidjson::Value& rapidJsonValueDestinations = rapidJsonValueTextureChannelPacking["Destinations"];
					if (rapidJsonValueDestinations.Size() > 4)
					{
						throw std::runtime_error("Texture channel packing \"" + textureChannelPacking + "\" has more than four destinations which is invalid");
					}
					mDestinations.resize(rapidJsonValueDestinations.Size());
					for (rapidjson::SizeType i = 0; i < rapidJsonValueDestinations.Size(); ++i)
					{
						const rapidjson::Value& rapidJsonValueDestination = rapidJsonValueDestinations[i];
						Destination& destination = mDestinations[i];
						{ // Get source index by texture semantic
							TextureSemantic textureSemantic = TextureSemantic::UNKNOWN;
							mandatoryTextureSemanticProperty(rapidJsonValueDestination, "TextureSemantic", textureSemantic);
							for (uint8_t sourceIndex = 0; sourceIndex < mSources.size(); ++sourceIndex)
							{
								if (mSources[sourceIndex].textureSemantic == textureSemantic)
								{
									destination.sourceIndex = sourceIndex;
									break;
								}
							}
							if (Renderer::isInvalid(destination.sourceIndex))
							{
								throw std::runtime_error("Texture channel packing \"" + textureChannelPacking + "\" destination " + std::to_string(i) + ": Found no texture channel packing source for the given texture semantic");
							}
						}
						destination.sourceChannel = static_cast<uint8_t>(rapidJsonValueDestination["SourceChannel"].GetUint());
						if (destination.sourceChannel > mSources[destination.sourceIndex].numberOfChannels)
						{
							throw std::runtime_error("Texture channel packing \"" + textureChannelPacking + "\" destination " + std::to_string(i) + " is referencing a source channel which doesn't exist");
						}
					}
				}

				{ // RGB hardware gamma correction used during runtime? (= sRGB)
				  // -> The "RgbHardwareGammaCorrection"-name was chosen to stay consistent to material blueprints (don't use too many different names for more or less the same topic)
					bool rgbHardwareGammaCorrection = false;
					RendererToolkit::JsonHelper::optionalBooleanProperty(rapidJsonValueTextureChannelPacking, "RgbHardwareGammaCorrection", rgbHardwareGammaCorrection);
					if (!rgbHardwareGammaCorrection)
					{
						crunchConvertParams.m_comp_params.set_flag(cCRNCompFlagPerceptual, false);
						crunchConvertParams.m_mipmap_params.m_gamma_filtering = false;
						crunchConvertParams.m_mipmap_params.m_gamma = 1.0f;	// Mipmap gamma correction value, default=2.2, use 1.0 for linear
					}
				}
			}

			[[nodiscard]] std::string getSourceNormalMapFilename(const char* basePath, Renderer::VirtualFilename virtualSourceNormalMapFilename, const rapidjson::Value& rapidJsonValueInputFiles) const
			{
				if (nullptr != virtualSourceNormalMapFilename)
				{
					// Use the normal map we received
					return virtualSourceNormalMapFilename;
				}
				else
				{
					// Search for a normal map inside the texture channel packing layout
					for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorInputFile = rapidJsonValueInputFiles.MemberBegin(); rapidJsonMemberIteratorInputFile != rapidJsonValueInputFiles.MemberEnd(); ++rapidJsonMemberIteratorInputFile)
					{
						if (getTextureSemanticByRapidJsonValue(rapidJsonMemberIteratorInputFile->name) == TextureSemantic::NORMAL_MAP)
						{
							return basePath + RendererToolkit::JsonHelper::getAssetFile(rapidJsonMemberIteratorInputFile->value);
						}
					}
				}

				// No normal map filename found
				return "";
			}

			void loadSourceCrunchMipmappedTextures(Renderer::IFileManager& fileManager, const rapidjson::Value& rapidJsonValueTextureAssetCompiler, const char* basePath, Renderer::VirtualFilename virtualSourceNormalMapFilename)
			{
				const bool toksvigSpecularAntiAliasing = isToksvigSpecularAntiAliasingEnabled(rapidJsonValueTextureAssetCompiler);

				// Load provided source textures
				const rapidjson::Value& rapidJsonValueInputFiles = rapidJsonValueTextureAssetCompiler["InputFiles"];
				if (rapidJsonValueInputFiles.MemberCount() == 0)
				{
					throw std::runtime_error("No input files defined");
				}
				bool roughnessMapFoundAndProcessed = false;
				for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorInputFile = rapidJsonValueInputFiles.MemberBegin(); rapidJsonMemberIteratorInputFile != rapidJsonValueInputFiles.MemberEnd(); ++rapidJsonMemberIteratorInputFile)
				{
					bool textureSemanticFound = false;
					const TextureSemantic textureSemantic = getTextureSemanticByRapidJsonValue(rapidJsonMemberIteratorInputFile->name);
					for (Source& source : mSources)
					{
						// We support automatic conversion between roughness map and gloss map
						const bool roughnessMapConversionNeeded = (TextureSemantic::ROUGHNESS_MAP == source.textureSemantic && TextureSemantic::GLOSS_MAP == textureSemantic) || (TextureSemantic::GLOSS_MAP == source.textureSemantic && TextureSemantic::ROUGHNESS_MAP == textureSemantic);
						if (source.textureSemantic == textureSemantic || roughnessMapConversionNeeded)
						{
							textureSemanticFound = true;
							const std::string value = RendererToolkit::JsonHelper::getAssetFile(rapidJsonMemberIteratorInputFile->value);

							// Sanity check: There's either a roughness map or a gloss map, but never ever both
							if (TextureSemantic::ROUGHNESS_MAP == textureSemantic || TextureSemantic::GLOSS_MAP == textureSemantic)
							{
								if (roughnessMapFoundAndProcessed)
								{
									throw std::runtime_error("Texture input file \"" + value + "\" with texture semantic \"" + std::string(rapidJsonMemberIteratorInputFile->name.GetString()) + "\": There's either a roughness map or a gloss map, but never ever both");
								}
								roughnessMapFoundAndProcessed = true;
							}

							// Support for Toksvig specular anti-aliasing to reduce shimmering
							std::string usedSourceNormalMapFilename;
							if ((textureSemantic == TextureSemantic::ROUGHNESS_MAP || textureSemantic == TextureSemantic::GLOSS_MAP) && toksvigSpecularAntiAliasing)
							{
								// Search for normal map
								usedSourceNormalMapFilename = getSourceNormalMapFilename(basePath, virtualSourceNormalMapFilename, rapidJsonValueInputFiles);
							}

							// Load Crunch mipmapped texture
							crnlib::texture_conversion::convert_params crunchConvertParams;
							load2DCrunchMipmappedTexture(fileManager, (basePath + value).c_str(), usedSourceNormalMapFilename.empty() ? nullptr : usedSourceNormalMapFilename.c_str(), source.crunchMipmappedTexture, crunchConvertParams);

							{ // Sanity check: Ensure the number of channels matches
								const crnlib::image_u8* crunchImage = source.crunchMipmappedTexture.get_level(0, 0)->get_image();
								for (uint8_t i = 0; i < source.numberOfChannels; ++i)
								{
									if (!crunchImage->is_component_valid(i))
									{
										throw std::runtime_error("Texture input file \"" + value + "\" has less channels then required by texture semantic \"" + std::string(rapidJsonMemberIteratorInputFile->name.GetString()) + '\"');
									}
								}
							}

							// We support automatic conversion between roughness map and gloss map
							if (roughnessMapConversionNeeded)
							{
								// Sanity check
								if (1 != source.numberOfChannels)
								{
									throw std::runtime_error("Texture input file \"" + value + "\" with texture semantic \"" + std::string(rapidJsonMemberIteratorInputFile->name.GetString()) + "\" must have exactly one channel");
								}

								// Convert
								const crnlib::mip_level& crunchMipLevel = *source.crunchMipmappedTexture.get_level(0, 0);
								const crnlib::uint width = crunchMipLevel.get_width();
								const crnlib::uint height = crunchMipLevel.get_height();
								crnlib::image_u8* crunchImage = crunchMipLevel.get_image();
								for (crnlib::uint y = 0; y < height; ++y)
								{
									for (crnlib::uint x = 0; x < width; ++x)
									{
										// Roughness = 1 - glossiness
										(*crunchImage)(x, y).c[0] = 255u - (*crunchImage)(x, y).c[0];
									}
								}
							}
							break;
						}
					}
					if (!textureSemanticFound)
					{
						throw std::runtime_error("Texture semantic \"" + std::string(rapidJsonMemberIteratorInputFile->name.GetString()) + "\" isn't defined inside texture channel packing \"" + rapidJsonValueTextureAssetCompiler["TextureChannelPacking"].GetString() + '\"');
					}
				}

				// Support for Toksvig specular anti-aliasing to reduce shimmering: Handle case if no roughness map to adjust was provided
				if (toksvigSpecularAntiAliasing)
				{
					for (Source& source : mSources)
					{
						if (source.textureSemantic == TextureSemantic::ROUGHNESS_MAP || source.textureSemantic == TextureSemantic::GLOSS_MAP)
						{
							if (!source.crunchMipmappedTexture.is_valid())
							{
								// Search for normal map
								const std::string usedSourceNormalMapFilename = getSourceNormalMapFilename(basePath, virtualSourceNormalMapFilename, rapidJsonValueInputFiles);
								if (!usedSourceNormalMapFilename.empty())
								{
									// Load Crunch mipmapped texture
									crnlib::texture_conversion::convert_params crunchConvertParams;
									load2DCrunchMipmappedTexture(fileManager, nullptr, usedSourceNormalMapFilename.c_str(), source.crunchMipmappedTexture, crunchConvertParams);
								}
							}
							break;
						}
					}
				}

				// Get combined maximum width and height of all source textures
				crnlib::uint maximumWidth = Renderer::getInvalid<crnlib::uint>();
				crnlib::uint maximumHeight = Renderer::getInvalid<crnlib::uint>();
				for (uint8_t i = 0; i < mSources.size(); ++i)
				{
					const Source& source = mSources[i];
					if (source.crunchMipmappedTexture.is_valid())
					{
						if (Renderer::isInvalid(maximumWidth) && Renderer::isInvalid(maximumHeight))
						{
							maximumWidth = source.crunchMipmappedTexture.get_width();
							maximumHeight = source.crunchMipmappedTexture.get_height();
						}
						else
						{
							maximumWidth = std::max(maximumWidth, source.crunchMipmappedTexture.get_width());
							maximumHeight = std::max(maximumHeight, source.crunchMipmappedTexture.get_height());
						}
					}
				}

				// Sanity check: All source textures must have the same size
				// -> The optional texture asset compiler option "ForceMaximumSizeUsage" can be used to enforce this, intentionally not enabled by default since the different size might have been an artist accident
				bool forceMaximumSizeUsage = false;
				RendererToolkit::JsonHelper::optionalBooleanProperty(rapidJsonValueTextureAssetCompiler, "ForceMaximumSizeUsage", forceMaximumSizeUsage);
				if (forceMaximumSizeUsage)
				{
					for (uint8_t i = 0; i < mSources.size(); ++i)
					{
						Source& source = mSources[i];
						crnlib::mipmapped_texture& crunchMipmappedTexture = source.crunchMipmappedTexture;
						if (crunchMipmappedTexture.is_valid() && (crunchMipmappedTexture.get_width() != maximumWidth || crunchMipmappedTexture.get_height() != maximumHeight))
						{
							crnlib::mipmapped_texture::resample_params crunchResampleParams;
							switch (source.textureSemantic)
							{
								case TextureSemantic::ALBEDO_MAP:
								case TextureSemantic::REFLECTION_2D_MAP:
								case TextureSemantic::EMISSIVE_MAP:
								case TextureSemantic::TERRAIN_HEIGHT_MAP:
								case TextureSemantic::REFLECTION_CUBE_MAP:
								case TextureSemantic::PACKED_CHANNELS:
								case TextureSemantic::CRN_ARRAY:
								case TextureSemantic::UNKNOWN:
									crunchResampleParams.m_srgb  = true;
									crunchResampleParams.m_gamma = 2.2f;	// Mipmap gamma correction value, default=2.2, use 1.0 for linear
									break;

								case TextureSemantic::ALPHA_MAP:
								case TextureSemantic::ROUGHNESS_MAP:
								case TextureSemantic::GLOSS_MAP:
								case TextureSemantic::METALLIC_MAP:
								case TextureSemantic::HEIGHT_MAP:
								case TextureSemantic::TINT_MAP:
								case TextureSemantic::AMBIENT_OCCLUSION_MAP:
								case TextureSemantic::COLOR_CORRECTION_LOOKUP_TABLE:
								case TextureSemantic::VOLUME:
								case TextureSemantic::IES_LIGHT_PROFILE_ARRAY:
									crunchResampleParams.m_gamma = 1.0f;	// Mipmap gamma correction value, default=2.2, use 1.0 for linear
									break;

								case TextureSemantic::NORMAL_MAP:
									crunchResampleParams.m_renormalize = true;
									crunchResampleParams.m_gamma	   = 1.0f;	// Mipmap gamma correction value, default=2.2, use 1.0 for linear
									break;
							}
							if (!crunchMipmappedTexture.resize(maximumWidth, maximumHeight, crunchResampleParams))
							{
								throw std::runtime_error("All input textures must have the same size, failed to automatically resize to the combined maximum width and height of all input textures " + std::to_string(maximumWidth) + 'x' + std::to_string(maximumHeight));
							}
						}
					}
				}
				else
				{
					for (uint8_t i = 0; i < mSources.size(); ++i)
					{
						const crnlib::mipmapped_texture& crunchSourceMipmappedTexture = mSources[i].crunchMipmappedTexture;
						if (crunchSourceMipmappedTexture.is_valid())
						{
							for (uint8_t k = i + 1u; k < static_cast<uint8_t>(mSources.size()); ++k)
							{
								const crnlib::mipmapped_texture& crunchOtherMipmappedTexture = mSources[k].crunchMipmappedTexture;
								if (crunchOtherMipmappedTexture.is_valid() && (crunchSourceMipmappedTexture.get_width() != crunchOtherMipmappedTexture.get_width() || crunchSourceMipmappedTexture.get_height() != crunchOtherMipmappedTexture.get_height()))
								{
									throw std::runtime_error("All input textures must have the same size. The combined maximum width and height of all input textures is " + std::to_string(maximumWidth) + 'x' + std::to_string(maximumHeight) + ". Set texture asset compiler option \"ForceMaximumSizeUsage\" to \"TRUE\" to enforce using this maximum size.");
								}
							}
							break;
						}
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Sources		 mSources;
			Destinations mDestinations;


		};


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		[[nodiscard]] bool crunchConsoleOutput(crnlib::eConsoleMessageType crunchType, const char* message, void* data)
		{
			// Map the log message type
			Rhi::ILog::Type type = Rhi::ILog::Type::TRACE;
			switch (crunchType)
			{
				case crnlib::cDebugConsoleMessage:
					type = Rhi::ILog::Type::DEBUG;
					break;

				case crnlib::cProgressConsoleMessage:
					// Ignored by intent since Crunch writes empty message here (search for "console::progress("");" inside Crunch source codes)
					// type = Rhi::ILog::Type::TRACE;
					break;

				case crnlib::cInfoConsoleMessage:
					type = Rhi::ILog::Type::INFORMATION;
					break;

				case crnlib::cConsoleConsoleMessage:
					type = Rhi::ILog::Type::INFORMATION;
					break;

				case crnlib::cMessageConsoleMessage:
					type = Rhi::ILog::Type::INFORMATION;
					break;

				case crnlib::cWarningConsoleMessage:
					type = Rhi::ILog::Type::WARNING;
					break;

				case crnlib::cErrorConsoleMessage:
					type = Rhi::ILog::Type::CRITICAL;
					break;

				case crnlib::cCMTTotal:
				default:
					break;
			}

			// Write RHI log
			// TODO(co) More context information like which asset is compiled right now might be useful. We need to keep in mind that there can be multiple texture compiler instances
			//          running at one and the same time. We could use the Crunch console output data to transport this information, on the other hand we need to ensure that we can
			//          unregister our function when we're done. "crnlib::console::remove_console_output_func() only checks the function pointer.
			if (static_cast<const RendererToolkit::Context*>(data)->getLog().print(type, nullptr, __FILE__, static_cast<uint32_t>(__LINE__), message))
			{
				DEBUG_BREAK;
			}

			// We handled the console output
			return true;
		}

		void* crunchRealloc(void* p, size_t size, size_t* pActual_size, bool, void* pUser_data)
		{
			if (nullptr != pActual_size)
			{
				*pActual_size = size;
			}
			return static_cast<Rhi::IAllocator*>(pUser_data)->reallocate(p, 0, size, CRNLIB_MIN_ALLOC_ALIGNMENT);
		}

		size_t crunchMsize(void*, void*)
		{
			throw std::runtime_error("\"crn_msize_func\" isn't supported, used only if \"CRNLIB_MEM_STATS\" preprocessor definition is set");
		}

		void initializeCrunch(const RendererToolkit::Context& context)
		{
			if (!g_CrunchInitialized)
			{
				// The Crunch console is using "printf()" by default if no console output function handles Crunch console output
				// -> Redirect the Crunch console output into our log so we have an uniform handling of such information
				crn_set_memory_callbacks(::detail::crunchRealloc, ::detail::crunchMsize, &context.getAllocator());
				crnlib::console::add_console_output_func(crunchConsoleOutput, &const_cast<RendererToolkit::Context&>(context));
				g_CrunchInitialized = true;
			}
		}

		void deinitializeCrunch()
		{
			if (g_CrunchInitialized)
			{
				crnlib::console::remove_console_output_func(crunchConsoleOutput);
				crnlib::console::deinit();
				crn_set_memory_callbacks(nullptr, nullptr, nullptr);
				g_CrunchInitialized = false;
			}
		}

		[[nodiscard]] std::string widthHeightToString(uint32_t width, uint32_t height)
		{
			return std::to_string(width) + 'x' + std::to_string(height);
		}

		void optionalTextureSemanticProperty(const rapidjson::Value& rapidJsonValue, const char* propertyName, TextureSemantic& value)
		{
			if (rapidJsonValue.HasMember(propertyName))
			{
				mandatoryTextureSemanticProperty(rapidJsonValue, propertyName, value);
			}
		}

		[[nodiscard]] Filenames getCubemapFilenames(const rapidjson::Value& rapidJsonValueTextureAssetCompiler, const std::string& basePath)
		{
			const rapidjson::Value& rapidJsonValueInputFiles = rapidJsonValueTextureAssetCompiler["InputFiles"];
			static constexpr uint32_t NUMBER_OF_FACES = 6;
			static constexpr char* FACE_NAMES[NUMBER_OF_FACES] = { "PositiveX", "NegativeX", "NegativeY", "PositiveY", "PositiveZ", "NegativeZ" };

			// The face order must be: +X, -X, -Y, +Y, +Z, -Z
			Filenames filenames;
			filenames.reserve(6);
			for (uint32_t faceIndex = 0; faceIndex < NUMBER_OF_FACES; ++faceIndex)
			{
				filenames.emplace_back(basePath + RendererToolkit::JsonHelper::getAssetInputFileByRapidJsonValue(rapidJsonValueInputFiles, FACE_NAMES[faceIndex]));
			}
			return filenames;
		}

		[[nodiscard]] bool checkIfChanged(const RendererToolkit::IAssetCompiler::Input& input, const RendererToolkit::IAssetCompiler::Configuration& configuration, const rapidjson::Value& rapidJsonValueTextureAssetCompiler, TextureSemantic textureSemantic, const std::string& virtualInputAssetFilename, const std::string& virtualOutputAssetFilename, std::vector<RendererToolkit::CacheManager::CacheEntries>& cacheEntries)
		{
			switch (textureSemantic)
			{
				case TextureSemantic::ROUGHNESS_MAP:
				case TextureSemantic::GLOSS_MAP:
				{
					// A roughness map has two source files: First the roughness map itself and second a normal map
					// -> An asset can specify both files or only one of them
					// -> "virtualInputAssetFilename" points to the roughness map
					// -> We need to fetch the name of the input normal map
					std::string virtualNormalMapAssetFilename;
					if (rapidJsonValueTextureAssetCompiler.HasMember("NormalMapInputFile"))
					{
						const std::string normalMapInputFile = rapidJsonValueTextureAssetCompiler["NormalMapInputFile"].GetString();
						if (!normalMapInputFile.empty())
						{
							virtualNormalMapAssetFilename = input.virtualAssetInputDirectory + '/' + normalMapInputFile;
						}
					}

					// Setup a list of source files
					Filenames virtualInputFilenames;
					if (!virtualInputAssetFilename.empty())
					{
						virtualInputFilenames.emplace_back(virtualInputAssetFilename);
					}
					if (!virtualNormalMapAssetFilename.empty())
					{
						virtualInputFilenames.emplace_back(virtualNormalMapAssetFilename);
					}

					RendererToolkit::CacheManager::CacheEntries cacheEntriesCandidate;
					if (input.cacheManager.needsToBeCompiled(configuration.rhiTarget, input.virtualAssetFilename, virtualInputFilenames, virtualOutputAssetFilename, TEXTURE_FORMAT_VERSION, cacheEntriesCandidate))
					{
						// Changed
						cacheEntries.push_back(cacheEntriesCandidate);
						return true;
					}

					// Not changed
					return false;
				}

				case TextureSemantic::REFLECTION_CUBE_MAP:
				{
					// A cube map has six source files (for each face one source), so check if any of the six files has been changed
					// -> "virtualInputAssetFilename" specifies the base directory of the faces source files
					const Filenames faceFilenames = getCubemapFilenames(rapidJsonValueTextureAssetCompiler, virtualInputAssetFilename);
					RendererToolkit::CacheManager::CacheEntries cacheEntriesCandidate;
					if (input.cacheManager.needsToBeCompiled(configuration.rhiTarget, input.virtualAssetFilename, faceFilenames, virtualOutputAssetFilename, TEXTURE_FORMAT_VERSION, cacheEntriesCandidate))
					{
						// Changed
						cacheEntries.push_back(cacheEntriesCandidate);
						return true;
					}

					// Not changed
					return false;
				}

				case TextureSemantic::PACKED_CHANNELS:
				{
					const rapidjson::Value& rapidJsonValueInputFiles = rapidJsonValueTextureAssetCompiler["InputFiles"];
					Filenames filenames;
					filenames.reserve(rapidJsonValueInputFiles.MemberCount());
					for (rapidjson::Value::ConstMemberIterator rapidJsonMemberIteratorInputFile = rapidJsonValueInputFiles.MemberBegin(); rapidJsonMemberIteratorInputFile != rapidJsonValueInputFiles.MemberEnd(); ++rapidJsonMemberIteratorInputFile)
					{
						filenames.emplace_back(virtualInputAssetFilename + RendererToolkit::JsonHelper::getAssetFile(rapidJsonMemberIteratorInputFile->value));
					}
					RendererToolkit::CacheManager::CacheEntries cacheEntriesCandidate;
					if (input.cacheManager.needsToBeCompiled(configuration.rhiTarget, input.virtualAssetFilename, filenames, virtualOutputAssetFilename, TEXTURE_FORMAT_VERSION, cacheEntriesCandidate))
					{
						// Changed
						cacheEntries.push_back(cacheEntriesCandidate);
						return true;
					}

					// Not changed
					return false;
				}

				case TextureSemantic::IES_LIGHT_PROFILE_ARRAY:
				case TextureSemantic::CRN_ARRAY:
				{
					const rapidjson::Value& rapidJsonValueInputFiles = rapidJsonValueTextureAssetCompiler["InputFiles"];
					Filenames filenames;
					const uint32_t numberOfFiles = rapidJsonValueInputFiles.Size();
					filenames.reserve(numberOfFiles);
					for (uint32_t i = 0; i < numberOfFiles; ++i)
					{
						filenames.emplace_back(virtualInputAssetFilename + RendererToolkit::JsonHelper::getAssetFile(rapidJsonValueInputFiles[i]));
					}
					RendererToolkit::CacheManager::CacheEntries cacheEntriesCandidate;
					if (input.cacheManager.needsToBeCompiled(configuration.rhiTarget, input.virtualAssetFilename, filenames, virtualOutputAssetFilename, RendererToolkit::IAssetCompiler::ASSET_FORMAT_VERSION, cacheEntriesCandidate))
					{
						// Changed
						cacheEntries.push_back(cacheEntriesCandidate);
						return true;
					}

					// Not changed
					return false;
				}

				case TextureSemantic::ALBEDO_MAP:
				case TextureSemantic::ALPHA_MAP:
				case TextureSemantic::NORMAL_MAP:
				case TextureSemantic::METALLIC_MAP:
				case TextureSemantic::EMISSIVE_MAP:
				case TextureSemantic::HEIGHT_MAP:
				case TextureSemantic::TERRAIN_HEIGHT_MAP:
				case TextureSemantic::TINT_MAP:
				case TextureSemantic::AMBIENT_OCCLUSION_MAP:
				case TextureSemantic::REFLECTION_2D_MAP:
				case TextureSemantic::COLOR_CORRECTION_LOOKUP_TABLE:
				case TextureSemantic::VOLUME:
				case TextureSemantic::UNKNOWN:
				default:
				{
					// Asset has single source file
					RendererToolkit::CacheManager::CacheEntries cacheEntriesCandidate;
					if (input.cacheManager.needsToBeCompiled(configuration.rhiTarget, input.virtualAssetFilename, virtualInputAssetFilename, virtualOutputAssetFilename, TEXTURE_FORMAT_VERSION, cacheEntriesCandidate))
					{
						// Changed
						cacheEntries.push_back(cacheEntriesCandidate);
						return true;
					}

					// Not changed
					return false;
				}
			}
		}

		void loadCubeCrunchMipmappedTexture(Renderer::IFileManager& fileManager, const rapidjson::Value& rapidJsonValueTextureAssetCompiler, const char* basePath, crnlib::mipmapped_texture& crunchMipmappedTexture)
		{
			// The face order must be: +X, -X, -Y, +Y, +Z, -Z
			const Filenames faceFilenames = getCubemapFilenames(rapidJsonValueTextureAssetCompiler, basePath);
			for (rapidjson::SizeType faceIndex = 0; faceIndex < faceFilenames.size(); ++faceIndex)
			{
				// Load the 2D source image
				const std::string& virtualInputFilename = faceFilenames[faceIndex];
				FileDataStreamSerializer fileDataStreamSerializer(fileManager, Renderer::IFileManager::FileMode::READ, virtualInputFilename.c_str());
				crnlib::image_u8* source2DImage = crnlib::crnlib_new<crnlib::image_u8>();
				if (!crnlib::image_utils::read_from_stream(*source2DImage, fileDataStreamSerializer))
				{
					throw std::runtime_error(std::string("Failed to load image \"") + virtualInputFilename + '\"');
				}

				// Sanity check
				const uint32_t width = source2DImage->get_width();
				if (width != source2DImage->get_height())
				{
					throw std::runtime_error("Cube map faces must have a width which is identical to the height");
				}

				// Process 2D source image
				const crnlib::pixel_format pixelFormat = source2DImage->has_alpha() ? crnlib::PIXEL_FMT_A8R8G8B8 : crnlib::PIXEL_FMT_R8G8B8;
				if (0 == faceIndex)
				{
					crunchMipmappedTexture.init(width, width, 1, 6, pixelFormat, "", crnlib::cDefaultOrientationFlags);
				}
				else if (crunchMipmappedTexture.get_format() != pixelFormat)
				{
					throw std::runtime_error("The pixel format of all cube map faces must be identical");
				}
				else if (crunchMipmappedTexture.get_width() != source2DImage->get_width())
				{
					throw std::runtime_error("The size of all cube map faces must be identical");
				}
				crunchMipmappedTexture.get_level(faceIndex, 0)->assign(source2DImage);
			}
		}

		void loadPackedChannelsCrunchMipmappedTexture(Renderer::IFileManager& fileManager, const RendererToolkit::IAssetCompiler::Configuration& configuration, const rapidjson::Value& rapidJsonValueTextureAssetCompiler, const char* basePath, Renderer::VirtualFilename virtualSourceNormalMapFilename, crnlib::mipmapped_texture& crunchMipmappedTexture, crnlib::texture_conversion::convert_params& crunchConvertParams)
		{
			// Load texture channel packing layout and source textures
			TextureChannelPacking textureChannelPacking(fileManager, configuration, rapidJsonValueTextureAssetCompiler, basePath, virtualSourceNormalMapFilename, crunchConvertParams);

			// Allocate the resulting Crunch mipmapped texture
			const crnlib::uint width = textureChannelPacking.getDestinationWidth();
			const crnlib::uint height = textureChannelPacking.getDestinationHeight();
			crunchMipmappedTexture.init(width, height, 1, 1, textureChannelPacking.getDestinationCrunchPixelFormat(), "Channel Packed Texture", crnlib::cDefaultOrientationFlags);

			// Fill the resulting Crunch mipmapped texture
			const TextureChannelPacking::Sources& sources = textureChannelPacking.getSources();
			const TextureChannelPacking::Destinations& destinations = textureChannelPacking.getDestinations();
			const crnlib::uint numberOfDestinationChannels = static_cast<crnlib::uint>(destinations.size());
			crnlib::image_u8* destinationCrunchImage = crunchMipmappedTexture.get_level(0, 0)->get_image();
			for (crnlib::uint destinationChannel = 0; destinationChannel < numberOfDestinationChannels; ++destinationChannel)
			{
				const TextureChannelPacking::Destination& destination = destinations[destinationChannel];
				const TextureChannelPacking::Source& source = sources[destination.sourceIndex];
				if (source.crunchMipmappedTexture.is_valid())
				{
					// Fill with source texture channel color
					const crnlib::image_u8* sourceCrunchImage = source.crunchMipmappedTexture.get_level(0, 0)->get_image();
					const uint8_t sourceChannel = destination.sourceChannel;
					for (crnlib::uint y = 0; y < height; ++y)
					{
						for (crnlib::uint x = 0; x < width; ++x)
						{
							(*destinationCrunchImage)(x, y).c[destinationChannel] = (*sourceCrunchImage)(x, y).c[sourceChannel];
						}
					}
				}
				else
				{
					// Fill with uniform default color
					const crnlib::uint8 value = static_cast<crnlib::uint8>(source.defaultColor[destination.sourceChannel] * 255.0f);
					for (crnlib::uint y = 0; y < height; ++y)
					{
						for (crnlib::uint x = 0; x < width; ++x)
						{
							(*destinationCrunchImage)(x, y).c[destinationChannel] = value;
						}
					}
				}
			}
		}

		void convertFile(const RendererToolkit::IAssetCompiler::Input& input, const RendererToolkit::IAssetCompiler::Configuration& configuration, const rapidjson::Value& rapidJsonValueTextureAssetCompiler, const char* basePath, Renderer::VirtualFilename virtualSourceFilename, Renderer::VirtualFilename virtualDestinationFilename, crnlib::texture_file_types::format outputCrunchTextureFileType, TextureSemantic textureSemantic, bool createMipmaps, float mipmapBlurriness, Renderer::VirtualFilename virtualSourceNormalMapFilename)
		{
			crnlib::texture_conversion::convert_params crunchConvertParams;

			// Load mipmapped Crunch texture
			crnlib::mipmapped_texture crunchMipmappedTexture;
			Renderer::IFileManager& fileManager = input.context.getFileManager();
			if (TextureSemantic::REFLECTION_CUBE_MAP == textureSemantic)
			{
				loadCubeCrunchMipmappedTexture(fileManager, rapidJsonValueTextureAssetCompiler, basePath, crunchMipmappedTexture);
				crunchConvertParams.m_texture_type = crnlib::cTextureTypeCubemap;
			}
			else if (TextureSemantic::PACKED_CHANNELS == textureSemantic)
			{
				loadPackedChannelsCrunchMipmappedTexture(fileManager, configuration, rapidJsonValueTextureAssetCompiler, basePath, virtualSourceNormalMapFilename, crunchMipmappedTexture, crunchConvertParams);
			}
			else
			{
				if (!isToksvigSpecularAntiAliasingEnabled(rapidJsonValueTextureAssetCompiler))
				{
					virtualSourceNormalMapFilename = nullptr;
				}
				load2DCrunchMipmappedTexture(fileManager, virtualSourceFilename, virtualSourceNormalMapFilename, crunchMipmappedTexture, crunchConvertParams);
			}

			// Get absolute destination filename
			const std::string absoluteDestinationFilename = fileManager.mapVirtualToAbsoluteFilename(Renderer::IFileManager::FileMode::WRITE, virtualDestinationFilename);
			if (absoluteDestinationFilename.empty())
			{
				throw std::runtime_error("Failed determine the absolute destination filename of the virtual destination filename \"" + std::string(virtualDestinationFilename) + '\"');
			}

			// Setup Crunch parameters
			MemoryStream memoryStream;
			crunchConvertParams.m_pInput_texture = &crunchMipmappedTexture;
			crunchConvertParams.m_dst_stream = (crnlib::texture_file_types::cFormatDDS == outputCrunchTextureFileType) ? &memoryStream : nullptr;
			crunchConvertParams.m_dst_filename = (crnlib::texture_file_types::cFormatDDS == outputCrunchTextureFileType) ? "MemoryStream" : absoluteDestinationFilename.c_str();
			crunchConvertParams.m_dst_file_type = outputCrunchTextureFileType;
			crunchConvertParams.m_y_flip = true;
			crunchConvertParams.m_no_stats = true;
			crunchConvertParams.m_dst_format = crnlib::PIXEL_FMT_INVALID;
			crunchConvertParams.m_comp_params.m_num_helper_threads = crnlib::g_number_of_processors - 1;

			// The 4x4 block size based DXT compression format has no support for 1D textures
			bool compression = true;
			RendererToolkit::JsonHelper::optionalBooleanProperty(configuration.rapidJsonDocumentAsset["Asset"]["Compiler"], "Compression", compression);
			if (!compression || crunchMipmappedTexture.get_width() == 1 || crunchMipmappedTexture.get_height() == 1)
			{
				crunchConvertParams.m_dst_format = crnlib::PIXEL_FMT_A8R8G8B8;
			}

			// Evaluate texture semantic and figure out whether or not the destination format will be DXT compressed
			bool dxtCompressed = (crnlib::pixel_format::PIXEL_FMT_INVALID == crunchConvertParams.m_dst_format) ? true : static_cast<bool>(crnlib::pixel_format_helpers::is_dxt(crunchConvertParams.m_dst_format));	// TODO(co) Buggy external function signature "int crnlib::pixel_format_helpers::is_dxt(pixel_format fmt)", should probably return boolean
			switch (textureSemantic)
			{
				case TextureSemantic::ALBEDO_MAP:
				case TextureSemantic::REFLECTION_2D_MAP:
					// Nothing here, just a regular texture
					break;

				case TextureSemantic::ALPHA_MAP:
					// Those settings avoid the visual alpha test problems described at
					// "The Witness - Explore an abandoned island." - "Computing Alpha Mipmaps" - http://the-witness.net/news/2010/09/computing-alpha-mipmaps/
					// -> The topic is also mentioned at "Anti-aliased Alpha Test: The Esoteric Alpha To Coverage" - https://medium.com/@bgolus/anti-aliased-alpha-test-the-esoteric-alpha-to-coverage-8b177335ae4f
					crunchConvertParams.m_comp_params.set_flag(cCRNCompFlagPerceptual, false);
					crunchConvertParams.m_mipmap_params.m_gamma_filtering = false;
					crunchConvertParams.m_mipmap_params.m_gamma = 1.0f;	// Mipmap gamma correction value, default=2.2, use 1.0 for linear
					break;

				case TextureSemantic::NORMAL_MAP:
					crunchConvertParams.m_texture_type = crnlib::cTextureTypeNormalMap;
					crunchConvertParams.m_comp_params.set_flag(cCRNCompFlagPerceptual, false);
					crunchConvertParams.m_mipmap_params.m_renormalize = true;
					crunchConvertParams.m_mipmap_params.m_gamma_filtering = false;
					crunchConvertParams.m_mipmap_params.m_gamma = 1.0f;	// Mipmap gamma correction value, default=2.2, use 1.0 for linear

					// Do never ever store normal maps standard DXT1 compressed to not get horrible artefact's due to compressing vector data using algorithms design for color data
					// -> See "Real-Time Normal Map DXT Compression" -> "3.3 Tangent-Space 3Dc" - http://www.nvidia.com/object/real-time-normal-map-dxt-compression.html
					if (crnlib::pixel_format::PIXEL_FMT_INVALID == crunchConvertParams.m_dst_format || crnlib::pixel_format::PIXEL_FMT_DXT1 == crunchConvertParams.m_dst_format)
					{
						crunchConvertParams.m_dst_format = crnlib::PIXEL_FMT_3DC;
						dxtCompressed = true;
					}
					break;

				case TextureSemantic::ROUGHNESS_MAP:
				case TextureSemantic::GLOSS_MAP:
				case TextureSemantic::METALLIC_MAP:
				case TextureSemantic::HEIGHT_MAP:
				case TextureSemantic::TINT_MAP:
				case TextureSemantic::AMBIENT_OCCLUSION_MAP:
					crunchConvertParams.m_comp_params.set_flag(cCRNCompFlagPerceptual, false);
					crunchConvertParams.m_mipmap_params.m_gamma_filtering = false;
					crunchConvertParams.m_mipmap_params.m_gamma = 1.0f;	// Mipmap gamma correction value, default=2.2, use 1.0 for linear
					break;

				case TextureSemantic::EMISSIVE_MAP:
					// Nothing here, just a regular texture
					break;

				case TextureSemantic::TERRAIN_HEIGHT_MAP:
				case TextureSemantic::REFLECTION_CUBE_MAP:
				case TextureSemantic::COLOR_CORRECTION_LOOKUP_TABLE:
				case TextureSemantic::PACKED_CHANNELS:
				case TextureSemantic::VOLUME:
				case TextureSemantic::IES_LIGHT_PROFILE_ARRAY:
					// Nothing here, handled elsewhere
					break;

				case TextureSemantic::CRN_ARRAY:
				case TextureSemantic::UNKNOWN:
					// Nothing here, just a regular texture
					break;
			}

			// 4x4 block size based DXT compression means the texture dimension must be a multiple of four, for all mipmaps if mipmaps are used
			if (dxtCompressed)
			{
				// Check base mipmap
				uint32_t width = crunchMipmappedTexture.get_width();
				uint32_t height = crunchMipmappedTexture.get_height();
				if (0 != (width % 4) || 0 != (height % 4))
				{
					throw std::runtime_error("4x4 block size based DXT compression used, but the texture dimension " + widthHeightToString(width, height) + " is no multiple of four");
				}
				else if (createMipmaps)
				{
					// Check mipmaps and at least inform in case dynamic texture resolution scale will be limited
					uint32_t mipmap = 0;
					while (width > 4 && height > 4)
					{
						// Check mipmap
						if (0 != (width % 4) || 0 != (height % 4))
						{
							const std::string warning = "4x4 block size based DXT compression used, but the texture dimension " + widthHeightToString(width, height) +
														" at mipmap level " + std::to_string(mipmap) + " is no multiple of four. Texture dimension is " +
														widthHeightToString(crunchMipmappedTexture.get_width(), crunchMipmappedTexture.get_height()) + ". Dynamic texture resolution scale will be limited to mipmap level " + std::to_string(mipmap - 1) + '.';
							RHI_LOG(input.context, WARNING, warning.c_str())
							break;
						}

						// Move on to the next mipmap and ensure the size is always at least 1x1
						++mipmap;
						width = Rhi::ITexture::getHalfSize(width);
						height = Rhi::ITexture::getHalfSize(height);
					}
				}
			}

			// Create mipmaps?
			crunchConvertParams.m_mipmap_params.m_mode = createMipmaps ? cCRNMipModeGenerateMips : cCRNMipModeNoMips;
			crunchConvertParams.m_mipmap_params.m_blurriness = mipmapBlurriness;

			// Evaluate the quality strategy
			switch (configuration.qualityStrategy)
			{
				case RendererToolkit::QualityStrategy::DEBUG:
					// Most aggressive option: Reduce texture size
					if (dxtCompressed)
					{
						// 4x4 block size based DXT compression means the texture dimension must be a multiple of four, for all mipmaps if mipmaps are used
						// -> Ensure we don't go below 4x4 to not get into troubles with 4x4 blocked based compression
						// -> Ensure the base mipmap we tell the RHI about is a multiple of four. Even if the original base mipmap is a multiple of four, one of the lower mipmaps might not be.
						const uint32_t width = crunchMipmappedTexture.get_width();
						const uint32_t height = crunchMipmappedTexture.get_height();
						static const int NUMBER_OF_TOP_MIPMAPS_TO_REMOVE = 2;
						int startLevelIndex = NUMBER_OF_TOP_MIPMAPS_TO_REMOVE;

						// First, try to remove more mipmaps without violating the DXT size restrictions
						while ((0 != (std::max(1U, width >> startLevelIndex) % 4) || (0 != std::max(1U, height >> startLevelIndex) % 4)) &&
							  (std::max(1U, width >> startLevelIndex) > 4 && std::max(1U, height >> startLevelIndex) > 4))
						{
							++startLevelIndex;
						}
						if (0 != (std::max(1U, width >> startLevelIndex) % 4) || (0 != std::max(1U, height >> startLevelIndex) % 4))
						{
							// Second, remove less mipmaps
							startLevelIndex = NUMBER_OF_TOP_MIPMAPS_TO_REMOVE;
							while (startLevelIndex > 0 && (std::max(1U, width >> startLevelIndex) < 4 || std::max(1U, height >> startLevelIndex) < 4))
							{
								--startLevelIndex;
							}
							while (startLevelIndex > 0 && (0 != (std::max(1U, width >> startLevelIndex) % 4) || (0 != std::max(1U, height >> startLevelIndex) % 4)))
							{
								--startLevelIndex;
							}
						}

						// Set Crunch parameters
						crunchConvertParams.m_mipmap_params.m_scale_mode = cCRNSMAbsolute;
						crunchConvertParams.m_mipmap_params.m_scale_x	 = static_cast<float>(std::max(1U, width >> startLevelIndex));
						crunchConvertParams.m_mipmap_params.m_scale_y	 = static_cast<float>(std::max(1U, height >> startLevelIndex));
					}
					else
					{
						// Set Crunch parameters
						crunchConvertParams.m_mipmap_params.m_scale_mode = cCRNSMRelative;
						crunchConvertParams.m_mipmap_params.m_scale_x	 = 0.25f;
						crunchConvertParams.m_mipmap_params.m_scale_y	 = 0.25f;
					}

					// Disable several output file optimizations
					crunchConvertParams.m_comp_params.set_flag(cCRNCompFlagQuick, true);

					// Set endpoint optimizer's maximum iteration depth
					crunchConvertParams.m_comp_params.m_dxt_quality = cCRNDXTQualitySuperFast;

					// Set clustered DDS/CRN quality factor [0-255] 255=best
					crunchConvertParams.m_comp_params.m_quality_level = cCRNMinQualityLevel;
					break;

				case RendererToolkit::QualityStrategy::PRODUCTION:
					// Set endpoint optimizer's maximum iteration depth
					crunchConvertParams.m_comp_params.m_dxt_quality = cCRNDXTQualityNormal;

					// Set clustered DDS/CRN quality factor [0-255] 255=best
					crunchConvertParams.m_comp_params.m_quality_level = (cCRNMaxQualityLevel - cCRNMinQualityLevel) / 2;
					break;

				case RendererToolkit::QualityStrategy::SHIPPING:
					// Set endpoint optimizer's maximum iteration depth
					crunchConvertParams.m_comp_params.m_dxt_quality = cCRNDXTQualityUber;

					// Set clustered DDS/CRN quality factor [0-255] 255=best
					crunchConvertParams.m_comp_params.m_quality_level = cCRNMaxQualityLevel;
					break;
			}

			// Silence "Target bitrate/quality level is not supported for this output file format." warnings
			if (crnlib::texture_file_types::format::cFormatDDS == outputCrunchTextureFileType || crnlib::texture_file_types::format::cFormatKTX == outputCrunchTextureFileType)
			{
				crunchConvertParams.m_comp_params.m_quality_level = cCRNMaxQualityLevel;
			}

			// Compress now
			crnlib::texture_conversion::convert_stats stats;
			if (!crnlib::texture_conversion::process(crunchConvertParams, stats))
			{
				if (crunchConvertParams.m_error_message.is_empty())
				{
					throw std::runtime_error("Failed writing output file \"" + std::string(virtualDestinationFilename) + '\"');
				}
				else
				{
					throw std::runtime_error(crunchConvertParams.m_error_message.get_ptr());
				}
			}

			// Write LZ4 compressed memory file
			if (crnlib::texture_file_types::cFormatDDS == outputCrunchTextureFileType && !memoryStream.getMemoryFile().writeLz4CompressedDataByVirtualFilename(Renderer::Lz4DdsTextureResourceLoader::FORMAT_TYPE, Renderer::Lz4DdsTextureResourceLoader::FORMAT_VERSION, input.context.getFileManager(), virtualDestinationFilename))
			{
				throw std::runtime_error("Failed to write LZ4 compressed output file \"" + std::string(virtualDestinationFilename) + '\"');
			}
		}

		void convertColorCorrectionLookupTable(Renderer::IFileManager& fileManager, Renderer::VirtualFilename virtualInputAssetFilename, Renderer::VirtualFilename virtualOutputAssetFilename)
		{
			// Load the 2D source image
			FileDataStreamSerializer fileDataStreamSerializer(fileManager, Renderer::IFileManager::FileMode::READ, virtualInputAssetFilename);
			crnlib::image_u8 sourceImage;
			crnlib::image_utils::read_from_stream(sourceImage, fileDataStreamSerializer);

			// Sanity checks
			if (sourceImage.get_width() < sourceImage.get_height())
			{
				throw std::runtime_error("Color correction lookup table width must be equal or greater as the height");
			}
			if (!sourceImage.has_rgb() || sourceImage.has_alpha())
			{
				throw std::runtime_error("Color correction lookup table must be RGB");
			}

			// Create the 3D texture destination data which always has four components per texel
			const uint32_t width = sourceImage.get_height();	// Each 3D texture layer is a square
			const uint32_t height = sourceImage.get_height();
			const uint32_t numberOfTexelsPerLayer = width * height;
			const uint32_t depth = sourceImage.get_width() / height;
			crnlib::color_quad_u8* destinationData = new crnlib::color_quad_u8[numberOfTexelsPerLayer * depth];
			{
				crnlib::color_quad_u8* currentDestinationData = destinationData;
				uint32_t sourceX = 0;
				for (uint32_t z = 0; z < depth; ++z, sourceX += width, currentDestinationData += numberOfTexelsPerLayer)
				{
					if (!sourceImage.extract_block(currentDestinationData, sourceX, 0, width, height))
					{
						throw std::runtime_error("Color correction lookup table failed to extract block");
					}
				}
			}

			// Fill dds header ("PIXEL_FMT_A8R8G8B8" pixel format)
			crnlib::DDSURFACEDESC2 ddsSurfaceDesc2 = {};
			ddsSurfaceDesc2.dwSize								= sizeof(crnlib::DDSURFACEDESC2);
			ddsSurfaceDesc2.dwFlags								= crnlib::DDSD_WIDTH | crnlib::DDSD_HEIGHT | crnlib::DDSD_DEPTH | crnlib::DDSD_PIXELFORMAT | crnlib::DDSD_CAPS | crnlib::DDSD_LINEARSIZE;
			ddsSurfaceDesc2.dwHeight							= height;
			ddsSurfaceDesc2.dwWidth								= width;
			ddsSurfaceDesc2.dwBackBufferCount					= depth;
			ddsSurfaceDesc2.ddsCaps.dwCaps						= crnlib::DDSCAPS_TEXTURE | crnlib::DDSCAPS_COMPLEX;
			ddsSurfaceDesc2.ddsCaps.dwCaps2						= crnlib::DDSCAPS2_VOLUME;
			ddsSurfaceDesc2.ddpfPixelFormat.dwSize				= sizeof(crnlib::DDPIXELFORMAT);
			ddsSurfaceDesc2.ddpfPixelFormat.dwFlags				= (crnlib::DDPF_RGB | crnlib::DDPF_ALPHAPIXELS);
			ddsSurfaceDesc2.ddpfPixelFormat.dwRGBBitCount		= 32;
			ddsSurfaceDesc2.ddpfPixelFormat.dwRBitMask			= 0xFF0000;
			ddsSurfaceDesc2.ddpfPixelFormat.dwGBitMask			= 0x00FF00;
			ddsSurfaceDesc2.ddpfPixelFormat.dwBBitMask			= 0x0000FF;
			ddsSurfaceDesc2.ddpfPixelFormat.dwRGBAlphaBitMask	= 0xFF000000;
			ddsSurfaceDesc2.lPitch								= static_cast<crn_int32>((ddsSurfaceDesc2.dwWidth * ddsSurfaceDesc2.ddpfPixelFormat.dwRGBBitCount) >> 3);

			// Write down the 3D destination texture
			Renderer::MemoryFile memoryFile(0, 4096);
			memoryFile.write("DDS ", sizeof(uint32_t));
			memoryFile.write(reinterpret_cast<const char*>(&ddsSurfaceDesc2), sizeof(crnlib::DDSURFACEDESC2));
			memoryFile.write(reinterpret_cast<const char*>(destinationData), sizeof(crnlib::color_quad_u8) * numberOfTexelsPerLayer * depth);
			if (!memoryFile.writeLz4CompressedDataByVirtualFilename(Renderer::Lz4DdsTextureResourceLoader::FORMAT_TYPE, Renderer::Lz4DdsTextureResourceLoader::FORMAT_VERSION, fileManager, virtualOutputAssetFilename))
			{
				delete [] destinationData;
				throw std::runtime_error("Failed to write to destination file \"" + std::string(virtualOutputAssetFilename) + '\"');
			}

			// Done
			delete [] destinationData;
		}

		void convertTerrainHeightMap(Renderer::IFileManager& fileManager, Renderer::VirtualFilename virtualInputAssetFilename, Renderer::VirtualFilename virtualOutputAssetFilename)
		{
			// Load the 2D source image
			FileDataStreamSerializer fileDataStreamSerializer(fileManager, Renderer::IFileManager::FileMode::READ, virtualInputAssetFilename);
			crnlib::uint8_vec buf;
			if (fileDataStreamSerializer.read_entire_file(buf))
			{
				int x = 0, y = 0, n = 0;
				stbi_set_flip_vertically_on_load(true);
				stbi_us* pData = stbi_load_16_from_memory(buf.get_ptr(), static_cast<int>(buf.size_in_bytes()), &x, &y, &n, 1);
				stbi_set_flip_vertically_on_load(false);
				if (nullptr != pData)
				{
					const uint32_t numberOfTexelsPerLayer = static_cast<uint32_t>(x * y);

					// TODO(co) Check n?

					// Fill dds header for 16-bit height map "DXGI_FORMAT_R16_UNORM" ("A single-component, 16-bit unsigned-normalized-integer format that supports 16 bits for the red channel.") used during runtime.
					// TODO(co) Correct this so generic dds tools can open the texture as well
					crnlib::DDSURFACEDESC2 ddsSurfaceDesc2 = {};
					ddsSurfaceDesc2.dwSize								= sizeof(crnlib::DDSURFACEDESC2);
					ddsSurfaceDesc2.dwFlags								= crnlib::DDSD_WIDTH | crnlib::DDSD_HEIGHT | crnlib::DDSD_PIXELFORMAT | crnlib::DDSD_CAPS | crnlib::DDSD_LINEARSIZE;
					ddsSurfaceDesc2.dwHeight							= static_cast<crn_uint32>(y);
					ddsSurfaceDesc2.dwWidth								= static_cast<crn_uint32>(x);
					ddsSurfaceDesc2.dwBackBufferCount					= 1;
					ddsSurfaceDesc2.ddsCaps.dwCaps						= crnlib::DDSCAPS_TEXTURE | crnlib::DDSCAPS_COMPLEX;
					ddsSurfaceDesc2.ddsCaps.dwCaps2						= 0;
					ddsSurfaceDesc2.ddpfPixelFormat.dwSize				= sizeof(crnlib::DDPIXELFORMAT);
					ddsSurfaceDesc2.ddpfPixelFormat.dwFlags				= crnlib::DDPF_LUMINANCE;
					ddsSurfaceDesc2.ddpfPixelFormat.dwRGBBitCount		= 32;
					ddsSurfaceDesc2.ddpfPixelFormat.dwRBitMask			= 0xFF0000;
					ddsSurfaceDesc2.ddpfPixelFormat.dwGBitMask			= 0x00FF00;
					ddsSurfaceDesc2.ddpfPixelFormat.dwBBitMask			= 0x0000FF;
					ddsSurfaceDesc2.ddpfPixelFormat.dwRGBAlphaBitMask	= 0xFF000000;
					ddsSurfaceDesc2.lPitch								= static_cast<crn_int32>((ddsSurfaceDesc2.dwWidth * ddsSurfaceDesc2.ddpfPixelFormat.dwRGBBitCount) >> 3);

					{ // Write down the 3D destination texture
						Renderer::MemoryFile memoryFile(0, 4096);
						memoryFile.write("DDS ", sizeof(uint32_t));
						memoryFile.write(reinterpret_cast<const char*>(&ddsSurfaceDesc2), sizeof(crnlib::DDSURFACEDESC2));
						memoryFile.write(pData, sizeof(stbi_us) * numberOfTexelsPerLayer);
						if (!memoryFile.writeLz4CompressedDataByVirtualFilename(Renderer::Lz4DdsTextureResourceLoader::FORMAT_TYPE, Renderer::Lz4DdsTextureResourceLoader::FORMAT_VERSION, fileManager, virtualOutputAssetFilename))
						{
							stbi_image_free(pData);
							throw std::runtime_error("Failed to write to destination file \"" + std::string(virtualOutputAssetFilename) + '\"');
						}
					}

					// Free temporary memory
					stbi_image_free(pData);
				}
				else
				{
					// Error! TODO(co) Handle
				}
			}
			else
			{
				// Error! TODO(co) Handle
				//throw std::runtime_error("Color correction lookup table must be RGB");
			}
		}

		/**
		*  @brief
		*    Primitive texture compiler implementation for the "RAW" volume data file format (Lookout! You have to provide correct data type, width, height and depth loader parameters!)
		*
		*  @note
		*    - Primitive chunk of a certain data type
		*    - Lookout! This loader requires the user to provide correct loader parameters! (data type, width, height and depth)
		*    - The image loader is only able to deal with the volumetric image data, not with volumetric specific additional information like voxel size
		*/
		void convertVolume(const RendererToolkit::IAssetCompiler::Configuration& configuration, Renderer::IFileManager& fileManager, Renderer::VirtualFilename virtualInputAssetFilename, Renderer::VirtualFilename virtualOutputAssetFilename)
		{
			// Get and check the filename extension
			std::string extension = std_filesystem::path(virtualInputAssetFilename).extension().generic_string();
			RendererToolkit::StringHelper::toLowerCase(extension);
			if (".raw" != extension)
			{
				throw std::runtime_error("Failed to convert volume " + std::string(virtualInputAssetFilename) + ": Only raw volume data is supported");
			}

			// Get the JSON "RawVolume" object
			const rapidjson::Value& rapidJsonValueTextureAssetCompiler = configuration.rapidJsonDocumentAsset["Asset"]["Compiler"];
			if (!rapidJsonValueTextureAssetCompiler.HasMember("RawVolume"))
			{
				throw std::runtime_error("Failed to convert volume " + std::string(virtualInputAssetFilename) + ": \"RawVolume\" block is missing inside the texture asset JSON file");
			}
			const rapidjson::Value& rapidJsonValueRawVolume = rapidJsonValueTextureAssetCompiler["RawVolume"];
			if (strcmp(rapidJsonValueRawVolume["Format"].GetString(), "UCHAR") != 0)
			{
				throw std::runtime_error("Failed to convert volume " + std::string(virtualInputAssetFilename) + ": Texture asset JSON file \"RawVolume\" format must be \"UCHAR\"");
			}

			// Get the resolution
			uint32_t resolution[3] = { 0, 0, 0 };
			{
				std::vector<std::string> elements;
				RendererToolkit::StringHelper::splitString(rapidJsonValueRawVolume["Resolution"].GetString(), ' ', elements);
				if (elements.size() != 3)
				{
					throw std::runtime_error("Failed to convert volume " + std::string(virtualInputAssetFilename) + ": Texture asset JSON file \"RawVolume\" resolution needs three components");
				}
				for (uint32_t i = 0; i < 3; ++i)
				{
					resolution[i] = static_cast<uint32_t>(std::atoi(elements[i].c_str()));
				}
			}

			// Read in the RAW volume data
			const uint32_t rawVoumeDataNumberOfBytes = resolution[0] * resolution[1] * resolution[2];
			std::vector<uint8_t> rawVoumeData;
			{
				Renderer::IFile* file = fileManager.openFile(Renderer::IFileManager::FileMode::READ, virtualInputAssetFilename);
				if (nullptr == file)
				{
					throw std::runtime_error("Failed to open source file \"" + std::string(virtualInputAssetFilename) + '\"');
				}
				rawVoumeData.resize(rawVoumeDataNumberOfBytes);
				file->read(rawVoumeData.data(), rawVoumeDataNumberOfBytes);
				fileManager.closeFile(*file);
			}

			// Fill dds header ("PIXEL_FMT_A8R8G8B8" pixel format)
			crnlib::DDSURFACEDESC2 ddsSurfaceDesc2 = {};
			ddsSurfaceDesc2.dwSize								= sizeof(crnlib::DDSURFACEDESC2);
			ddsSurfaceDesc2.dwFlags								= crnlib::DDSD_WIDTH | crnlib::DDSD_HEIGHT | crnlib::DDSD_DEPTH | crnlib::DDSD_PIXELFORMAT | crnlib::DDSD_CAPS | crnlib::DDSD_LINEARSIZE;
			ddsSurfaceDesc2.dwHeight							= resolution[1];
			ddsSurfaceDesc2.dwWidth								= resolution[0];
			ddsSurfaceDesc2.dwBackBufferCount					= resolution[2];
			ddsSurfaceDesc2.ddsCaps.dwCaps						= crnlib::DDSCAPS_TEXTURE | crnlib::DDSCAPS_COMPLEX;
			ddsSurfaceDesc2.ddsCaps.dwCaps2						= crnlib::DDSCAPS2_VOLUME;
			ddsSurfaceDesc2.ddpfPixelFormat.dwSize				= sizeof(crnlib::DDPIXELFORMAT);
			ddsSurfaceDesc2.ddpfPixelFormat.dwFlags				= crnlib::DDPF_LUMINANCE;
			ddsSurfaceDesc2.ddpfPixelFormat.dwRGBBitCount		= 8;
			ddsSurfaceDesc2.ddpfPixelFormat.dwRBitMask			= 0xFF0000;
			ddsSurfaceDesc2.ddpfPixelFormat.dwGBitMask			= 0x00FF00;
			ddsSurfaceDesc2.ddpfPixelFormat.dwBBitMask			= 0x0000FF;
			ddsSurfaceDesc2.ddpfPixelFormat.dwRGBAlphaBitMask	= 0xFF000000;
			ddsSurfaceDesc2.lPitch								= static_cast<crn_int32>((ddsSurfaceDesc2.dwWidth * ddsSurfaceDesc2.ddpfPixelFormat.dwRGBBitCount) >> 3);

			{ // Write down the 3D destination texture
				Renderer::MemoryFile memoryFile(0, 4096);
				memoryFile.write("DDS ", sizeof(uint32_t));
				memoryFile.write(reinterpret_cast<const char*>(&ddsSurfaceDesc2), sizeof(crnlib::DDSURFACEDESC2));
				memoryFile.write(rawVoumeData.data(), rawVoumeDataNumberOfBytes);
				if (!memoryFile.writeLz4CompressedDataByVirtualFilename(Renderer::Lz4DdsTextureResourceLoader::FORMAT_TYPE, Renderer::Lz4DdsTextureResourceLoader::FORMAT_VERSION, fileManager, virtualOutputAssetFilename))
				{
					throw std::runtime_error("Failed to write to destination file \"" + std::string(virtualOutputAssetFilename) + '\"');
				}
			}
		}

		/**
		*  @brief
		*    Texture compiler implementation for Illuminating Engineering Society (IES) light profile (photometric light data, use e.g. IESviewer ( http://photometricviewer.com/ ) as viewer)
		*/
		void convertIesLightProfile(const RendererToolkit::IAssetCompiler::Input& input, const RendererToolkit::IAssetCompiler::Configuration& configuration, Renderer::VirtualFilename virtualOutputAssetFilename)
		{
			// Get the JSON "IesLightProfile" object
			uint32_t resolution[2] = { 256, 1 };
			const rapidjson::Value& rapidJsonValueTextureAssetCompiler = configuration.rapidJsonDocumentAsset["Asset"]["Compiler"];
			if (rapidJsonValueTextureAssetCompiler.HasMember("IesLightProfile"))
			{
				const rapidjson::Value& rapidJsonValueIesLightProfile = rapidJsonValueTextureAssetCompiler["IesLightProfile"];

				{ // Get the resolution
					std::vector<std::string> elements;
					RendererToolkit::StringHelper::splitString(rapidJsonValueIesLightProfile["Resolution"].GetString(), ' ', elements);
					if (elements.size() != 2)
					{
						throw std::runtime_error("Failed to convert IES light profile " + std::string(input.virtualAssetFilename) + ": Texture asset JSON file \"IesLightProfile\" resolution needs two components");
					}
					for (uint32_t i = 0; i < 2; ++i)
					{
						resolution[i] = static_cast<uint32_t>(std::atoi(elements[i].c_str()));
					}
					if (resolution[1] != 1)
					{
						throw std::runtime_error("Failed to convert IES light profile " + std::string(input.virtualAssetFilename) + ": Currently only 1D IES light profiles are supported, height must be one");
					}
				}
			}

			// Read in the IES light profile data using the external library "ies" ( https://github.com/ray-cast/ies )
			const rapidjson::Value& rapidJsonValueInputFiles = rapidJsonValueTextureAssetCompiler["InputFiles"];
			struct IESOutputData final
			{
				uint32_t width;
				uint32_t height;
				uint8_t channel;
				std::vector<float> stream;
			};
			std::vector<IESOutputData> iesOutputData;
			const uint32_t numberOfFiles = rapidJsonValueInputFiles.Size();
			iesOutputData.resize(numberOfFiles);
			Renderer::IFileManager& fileManager = input.context.getFileManager();
			for (uint32_t i = 0; i < numberOfFiles; ++i)
			{
				// Get virtual input asset filename
				const std::string inputFile = rapidJsonValueInputFiles[i].GetString();
				std::string extension = std_filesystem::path(inputFile).extension().generic_string();
				RendererToolkit::StringHelper::toLowerCase(extension);
				if (".ies" != extension)
				{
					throw std::runtime_error("Failed to convert IES light profile " + std::string(inputFile) + ": Only IES light profile data is supported");
				}
				const std::string virtualInputAssetFilename = input.virtualAssetInputDirectory + '/' + inputFile;

				// Load file content into memory
				std::vector<char> iesBuffer;
				{
					Renderer::IFile* file = fileManager.openFile(Renderer::IFileManager::FileMode::READ, virtualInputAssetFilename.c_str());
					if (nullptr == file)
					{
						throw std::runtime_error("Failed to open source file \"" + std::string(virtualInputAssetFilename) + '\"');
					}
					const std::size_t numberOfFileBytes = file->getNumberOfBytes();
					iesBuffer.resize(numberOfFileBytes);
					file->read(iesBuffer.data(), numberOfFileBytes);
					fileManager.closeFile(*file);
				}

				// Load IES light profile
				IESLoadHelper iesLoadHelper;
				IESFileInfo iesFileInfo;
				if (!iesLoadHelper.load(iesBuffer.data(), iesBuffer.size(), iesFileInfo))
				{
					throw std::runtime_error("Failed to load IES light profile content from \"" + std::string(virtualInputAssetFilename) + "\": " + iesFileInfo.error());
				}

				// Convert IES light profile to 1D texture data
				IESOutputData& currentIesOutputData = iesOutputData[i];
				currentIesOutputData.width = resolution[0];
				currentIesOutputData.height = 1;
				currentIesOutputData.channel = 1;
				currentIesOutputData.stream.resize(currentIesOutputData.width * currentIesOutputData.channel);
				if (!iesLoadHelper.saveAs1D(iesFileInfo, currentIesOutputData.stream.data(), currentIesOutputData.width, currentIesOutputData.channel))
				{
					throw std::runtime_error("Failed to convert IES light profile content from \"" + std::string(virtualInputAssetFilename) + '\"');
				}
			}

			// Fill dds header ("DXGI_FORMAT_R32_FLOAT" pixel format)
			crnlib::DDSURFACEDESC2 ddsSurfaceDesc2 = {};
			ddsSurfaceDesc2.dwSize								= sizeof(crnlib::DDSURFACEDESC2);
			ddsSurfaceDesc2.dwFlags								= crnlib::DDSD_WIDTH | crnlib::DDSD_PIXELFORMAT | crnlib::DDSD_CAPS | crnlib::DDSD_LINEARSIZE;
			ddsSurfaceDesc2.dwHeight							= resolution[1];
			ddsSurfaceDesc2.dwWidth								= resolution[0];
			ddsSurfaceDesc2.dwBackBufferCount					= 1;
			ddsSurfaceDesc2.ddsCaps.dwCaps						= crnlib::DDSCAPS_TEXTURE | crnlib::DDSCAPS_COMPLEX;
			ddsSurfaceDesc2.ddsCaps.dwCaps2						= 0;
			ddsSurfaceDesc2.ddpfPixelFormat.dwSize				= sizeof(crnlib::DDPIXELFORMAT);
			ddsSurfaceDesc2.ddpfPixelFormat.dwFlags				= crnlib::DDPF_LUMINANCE | crnlib::DDPF_FOURCC;
			ddsSurfaceDesc2.ddpfPixelFormat.dwFourCC			= CRNLIB_PIXEL_FMT_FOURCC('D', 'X', '1', '0');
			ddsSurfaceDesc2.ddpfPixelFormat.dwRGBBitCount		= 32;
			ddsSurfaceDesc2.ddpfPixelFormat.dwRBitMask			= 0xFF0000;
			ddsSurfaceDesc2.ddpfPixelFormat.dwGBitMask			= 0x00FF00;
			ddsSurfaceDesc2.ddpfPixelFormat.dwBBitMask			= 0x0000FF;
			ddsSurfaceDesc2.ddpfPixelFormat.dwRGBAlphaBitMask	= 0xFF000000;
			ddsSurfaceDesc2.lPitch								= static_cast<crn_int32>((ddsSurfaceDesc2.dwWidth * ddsSurfaceDesc2.ddpfPixelFormat.dwRGBBitCount) >> 3);

			// Fill dds DX10 header
			::detail::DdsHeaderDX10 ddsHeaderDX10 = {};
			ddsHeaderDX10.DXGIFormat = 41;	// DXGI_FORMAT_R32_FLOAT
			ddsHeaderDX10.arraySize  = numberOfFiles;

			{ // Write down the 1D destination texture
				Renderer::MemoryFile memoryFile(0, 4096);
				memoryFile.write("DDS ", sizeof(uint32_t));
				memoryFile.write(reinterpret_cast<const char*>(&ddsSurfaceDesc2), sizeof(crnlib::DDSURFACEDESC2));
				memoryFile.write(reinterpret_cast<const char*>(&ddsHeaderDX10), sizeof(::detail::DdsHeaderDX10));
				for (uint32_t i = 0; i < numberOfFiles; ++i)
				{
					const IESOutputData& currentIesOutputData = iesOutputData[i];
					memoryFile.write(currentIesOutputData.stream.data(), currentIesOutputData.stream.size() * sizeof(float));
				}
				if (!memoryFile.writeLz4CompressedDataByVirtualFilename(Renderer::Lz4DdsTextureResourceLoader::FORMAT_TYPE, Renderer::Lz4DdsTextureResourceLoader::FORMAT_VERSION, fileManager, virtualOutputAssetFilename))
				{
					throw std::runtime_error("Failed to write to destination file \"" + std::string(virtualOutputAssetFilename) + '\"');
				}
			}
		}

		/**
		*  @brief
		*    Convert CRN array
		*/
		void convertCrnArray(const RendererToolkit::IAssetCompiler::Input& input, const RendererToolkit::IAssetCompiler::Configuration& configuration, Renderer::VirtualFilename virtualOutputAssetFilename)
		{
			// Read in the texture asset IDs
			const rapidjson::Value& rapidJsonValueInputFiles = configuration.rapidJsonDocumentAsset["Asset"]["Compiler"]["InputFiles"];
			const uint32_t numberOfFiles = rapidJsonValueInputFiles.Size();
			std::vector<Renderer::AssetId> assetIds;
			assetIds.resize(numberOfFiles);
			for (uint32_t i = 0; i < numberOfFiles; ++i)
			{
				assetIds[i] = RendererToolkit::StringHelper::getAssetIdByString(rapidJsonValueInputFiles[i].GetString(), input);

				// TODO(co) Add CRN array sanity checks: The referenced texture asset must be CRN, all referenced texture assets must have the same size and same format, all referenced texture assets must be compiled texture assets (runtime generated texture assets are not supported)
			}

			{ // Write down the CRN array destination texture
				Renderer::MemoryFile memoryFile(0, 1024);
				memoryFile.write(&numberOfFiles, sizeof(uint32_t));
				memoryFile.write(assetIds.data(), sizeof(Renderer::AssetId) * numberOfFiles);
				if (!memoryFile.writeLz4CompressedDataByVirtualFilename(Renderer::v1CrnArray::FORMAT_TYPE, Renderer::v1CrnArray::FORMAT_VERSION, input.context.getFileManager(), virtualOutputAssetFilename))
				{
					throw std::runtime_error("Failed to write to destination file \"" + std::string(virtualOutputAssetFilename) + '\"');
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
	TextureAssetCompiler::TextureAssetCompiler(const Context& context)
	{
		::detail::initializeCrunch(context);
	}

	TextureAssetCompiler::~TextureAssetCompiler()
	{
		::detail::deinitializeCrunch();
	}


	//[-------------------------------------------------------]
	//[ Public virtual RendererToolkit::IAssetCompiler methods ]
	//[-------------------------------------------------------]
	std::string TextureAssetCompiler::getVirtualOutputAssetFilename(const Input& input, const Configuration& configuration) const
	{
		const rapidjson::Value& rapidJsonValueTextureAssetCompiler = configuration.rapidJsonDocumentAsset["Asset"]["Compiler"];
		::detail::TextureSemantic textureSemantic = ::detail::TextureSemantic::UNKNOWN;
		::detail::optionalTextureSemanticProperty(rapidJsonValueTextureAssetCompiler, "TextureSemantic", textureSemantic);
		std::string assetFileFormat;
		if (rapidJsonValueTextureAssetCompiler.HasMember("FileFormat"))
		{
			assetFileFormat = rapidJsonValueTextureAssetCompiler["FileFormat"].GetString();
		}
		if (::detail::TextureSemantic::COLOR_CORRECTION_LOOKUP_TABLE == textureSemantic || ::detail::TextureSemantic::TERRAIN_HEIGHT_MAP == textureSemantic || ::detail::TextureSemantic::VOLUME == textureSemantic || ::detail::TextureSemantic::IES_LIGHT_PROFILE_ARRAY == textureSemantic)
		{
			assetFileFormat = "lz4dds";
		}
		else if (::detail::TextureSemantic::CRN_ARRAY == textureSemantic)
		{
			assetFileFormat = "crn_array";
		}
		const std::string assetName = std_filesystem::path(input.virtualAssetFilename).stem().generic_string();
		std::string virtualOutputAssetFilename;
		crnlib::texture_file_types::format crunchOutputTextureFileType = crnlib::texture_file_types::cFormatCRN;
		::detail::getVirtualOutputAssetFilenameAndCrunchOutputTextureFileType(configuration, assetFileFormat, assetName, input.virtualAssetOutputDirectory, virtualOutputAssetFilename, crunchOutputTextureFileType);
		return virtualOutputAssetFilename;
	}

	bool TextureAssetCompiler::checkIfChanged(const Input& input, const Configuration& configuration) const
	{
		const rapidjson::Value& rapidJsonValueTextureAssetCompiler = configuration.rapidJsonDocumentAsset["Asset"]["Compiler"];
		std::string inputFile;
		if (rapidJsonValueTextureAssetCompiler.HasMember("InputFile"))
		{
			inputFile = JsonHelper::getAssetInputFileByRapidJsonValue(rapidJsonValueTextureAssetCompiler);
		}
		::detail::TextureSemantic textureSemantic = ::detail::TextureSemantic::UNKNOWN;
		::detail::optionalTextureSemanticProperty(rapidJsonValueTextureAssetCompiler, "TextureSemantic", textureSemantic);
		std::vector<CacheManager::CacheEntries> cacheEntries;
		return ::detail::checkIfChanged(input, configuration, rapidJsonValueTextureAssetCompiler, textureSemantic, input.virtualAssetInputDirectory + '/' + inputFile, getVirtualOutputAssetFilename(input, configuration), cacheEntries);
	}

	void TextureAssetCompiler::compile(const Input& input, const Configuration& configuration) const
	{
		// Read texture asset compiler configuration
		std::string inputFile;
		std::string assetFileFormat;
		::detail::TextureSemantic textureSemantic = ::detail::TextureSemantic::UNKNOWN;
		bool createMipmaps = true;
		float mipmapBlurriness = 0.9f;	// Scale filter kernel, >1=blur, <1=sharpen, .01-8, default=.9, Crunch default "blurriness" factor of 0.9 actually sharpens the output a little
		std::string normalMapInputFile;
		const rapidjson::Value& rapidJsonValueTextureAssetCompiler = configuration.rapidJsonDocumentAsset["Asset"]["Compiler"];
		{
			::detail::optionalTextureSemanticProperty(rapidJsonValueTextureAssetCompiler, "TextureSemantic", textureSemantic);
			if (rapidJsonValueTextureAssetCompiler.HasMember("InputFile"))
			{
				inputFile = JsonHelper::getAssetInputFileByRapidJsonValue(rapidJsonValueTextureAssetCompiler);
			}
			if (rapidJsonValueTextureAssetCompiler.HasMember("FileFormat"))
			{
				assetFileFormat = rapidJsonValueTextureAssetCompiler["FileFormat"].GetString();
			}
			JsonHelper::optionalBooleanProperty(rapidJsonValueTextureAssetCompiler, "CreateMipmaps", createMipmaps);
			JsonHelper::optionalFloatProperty(rapidJsonValueTextureAssetCompiler, "MipmapBlurriness", mipmapBlurriness);
			if (rapidJsonValueTextureAssetCompiler.HasMember("NormalMapInputFile"))
			{
				normalMapInputFile = rapidJsonValueTextureAssetCompiler["NormalMapInputFile"].GetString();
			}

			// Texture semantic overrules manual settings
			if (::detail::TextureSemantic::COLOR_CORRECTION_LOOKUP_TABLE == textureSemantic || ::detail::TextureSemantic::TERRAIN_HEIGHT_MAP == textureSemantic || ::detail::TextureSemantic::VOLUME == textureSemantic || ::detail::TextureSemantic::IES_LIGHT_PROFILE_ARRAY == textureSemantic)
			{
				assetFileFormat = "lz4dds";
				createMipmaps = false;
			}
			else if (::detail::TextureSemantic::CRN_ARRAY == textureSemantic)
			{
				assetFileFormat = "crn_array";
			}
		}
		const std::string& virtualAssetInputDirectory = input.virtualAssetInputDirectory;
		const std::string virtualInputAssetFilename = virtualAssetInputDirectory + '/' + inputFile;
		const std::string virtualNormalMapAssetFilename = virtualAssetInputDirectory + '/' + normalMapInputFile;
		const std::string assetName = std_filesystem::path(input.virtualAssetFilename).stem().generic_string();

		// Sanity checks
		if (inputFile.empty())
		{
			bool throwException = true;
			if (::detail::TextureSemantic::REFLECTION_CUBE_MAP == textureSemantic || ::detail::TextureSemantic::PACKED_CHANNELS == textureSemantic)
			{
				// Reflection cube maps or packed channels don't have a single input file, they're composed of multiple input files
				throwException = false;
			}
			else if (::detail::TextureSemantic::ROUGHNESS_MAP == textureSemantic || ::detail::TextureSemantic::GLOSS_MAP == textureSemantic)
			{
				// If a normal map input file is provided roughness maps can be calculated automatically using Toksvig specular anti-aliasing to reduce shimmering, in this case a input file is optional
				throwException = normalMapInputFile.empty();
			}
			else if (::detail::TextureSemantic::IES_LIGHT_PROFILE_ARRAY == textureSemantic || ::detail::TextureSemantic::CRN_ARRAY == textureSemantic)
			{
				// IES light profile array and CRN array textures don't have a single input file, they're composed of multiple input files
				throwException = false;
			}
			if (throwException)
			{
				throw std::runtime_error("Input file must be defined");
			}
		}
		if (::detail::TextureSemantic::ROUGHNESS_MAP != textureSemantic && ::detail::TextureSemantic::GLOSS_MAP != textureSemantic && ::detail::TextureSemantic::PACKED_CHANNELS != textureSemantic && !normalMapInputFile.empty())
		{
			throw std::runtime_error("Providing a normal map is only valid for roughness maps or packed channels");
		}

		// Get output related settings
		std::string virtualOutputAssetFilename;
		crnlib::texture_file_types::format crunchOutputTextureFileType = crnlib::texture_file_types::cFormatCRN;
		::detail::getVirtualOutputAssetFilenameAndCrunchOutputTextureFileType(configuration, assetFileFormat, assetName, input.virtualAssetOutputDirectory, virtualOutputAssetFilename, crunchOutputTextureFileType);

		// Ask the cache manager whether or not we need to compile the source file (e.g. source changed or target not there)
		std::vector<CacheManager::CacheEntries> cacheEntries;
		if (::detail::checkIfChanged(input, configuration, rapidJsonValueTextureAssetCompiler, textureSemantic, virtualInputAssetFilename, virtualOutputAssetFilename, cacheEntries))
		{
			switch (textureSemantic)
			{
				case ::detail::TextureSemantic::TERRAIN_HEIGHT_MAP:
					detail::convertTerrainHeightMap(input.context.getFileManager(), virtualInputAssetFilename.c_str(), virtualOutputAssetFilename.c_str());
					break;

				case ::detail::TextureSemantic::COLOR_CORRECTION_LOOKUP_TABLE:
					detail::convertColorCorrectionLookupTable(input.context.getFileManager(), virtualInputAssetFilename.c_str(), virtualOutputAssetFilename.c_str());
					break;

				case ::detail::TextureSemantic::VOLUME:
					detail::convertVolume(configuration, input.context.getFileManager(), virtualInputAssetFilename.c_str(), virtualOutputAssetFilename.c_str());
					break;

				case ::detail::TextureSemantic::IES_LIGHT_PROFILE_ARRAY:
					detail::convertIesLightProfile(input, configuration, virtualOutputAssetFilename.c_str());
					break;

				case ::detail::TextureSemantic::CRN_ARRAY:
					detail::convertCrnArray(input, configuration, virtualOutputAssetFilename.c_str());
					break;

				case ::detail::TextureSemantic::ALBEDO_MAP:
				case ::detail::TextureSemantic::ALPHA_MAP:
				case ::detail::TextureSemantic::NORMAL_MAP:
				case ::detail::TextureSemantic::ROUGHNESS_MAP:
				case ::detail::TextureSemantic::GLOSS_MAP:
				case ::detail::TextureSemantic::METALLIC_MAP:
				case ::detail::TextureSemantic::EMISSIVE_MAP:
				case ::detail::TextureSemantic::HEIGHT_MAP:
				case ::detail::TextureSemantic::TINT_MAP:
				case ::detail::TextureSemantic::AMBIENT_OCCLUSION_MAP:
				case ::detail::TextureSemantic::REFLECTION_2D_MAP:
				case ::detail::TextureSemantic::REFLECTION_CUBE_MAP:
				case ::detail::TextureSemantic::PACKED_CHANNELS:
				case ::detail::TextureSemantic::UNKNOWN:
				default:
					detail::convertFile(input, configuration, rapidJsonValueTextureAssetCompiler, virtualInputAssetFilename.c_str(), inputFile.empty() ? nullptr : virtualInputAssetFilename.c_str(), virtualOutputAssetFilename.c_str(), crunchOutputTextureFileType, textureSemantic, createMipmaps, mipmapBlurriness, normalMapInputFile.empty() ? nullptr : virtualNormalMapAssetFilename.c_str());
					break;
			}

			// Store new cache entries or update existing ones
			for (const CacheManager::CacheEntries& currentCacheEntries : cacheEntries)
			{
				input.cacheManager.storeOrUpdateCacheEntries(currentCacheEntries);
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
