#/*********************************************************\
# * Copyright (c) 2012-2021 The Unrimp Team
# *
# * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
# * and associated documentation files (the "Software"), to deal in the Software without
# * restriction, including without limitation the rights to use, copy, modify, merge, publish,
# * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
# * Software is furnished to do so, subject to the following conditions:
# *
# * The above copyright notice and this permission notice shall be included in all copies or
# * substantial portions of the Software.
# *
# * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#\*********************************************************/


##################################################
## CMake
##################################################
cmake_minimum_required(VERSION 3.14.0)


##################################################
## Preprocessor definitions
##################################################
add_definitions(-DGLM_FORCE_CXX2A -DGLM_FORCE_INLINE -DGLM_FORCE_AVX2 -DGLM_FORCE_QUAT_DATA_XYZW -DGLM_FORCE_LEFT_HANDED -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_RADIANS -DGLM_ENABLE_EXPERIMENTAL -DGLM_FORCE_SILENT_WARNINGS)
unrimp_add_conditional_rhi_definitions()


##################################################
## Includes
##################################################
include_directories(${CMAKE_SOURCE_DIR}/Source)
include_directories(${CMAKE_SOURCE_DIR}/External/Renderer)	# For "glm", "MikkTSpace" and "acl"
include_directories(${CMAKE_SOURCE_DIR}/External/Renderer/crunch/inc)
include_directories(${CMAKE_SOURCE_DIR}/External/Renderer/acl)
include_directories(${CMAKE_SOURCE_DIR}/External/RendererToolkit)	# For "SimpleFileWatcher" and "ies"
include_directories(${CMAKE_SOURCE_DIR}/External/RendererToolkit/RapidJSON/configuration)	# RapidJSON configuration and assert to exception
include_directories(${CMAKE_SOURCE_DIR}/External/RendererToolkit/RapidJSON/include)
include_directories(${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/include)
include_directories(${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/code)	# We want to be able to call e.g. "Assimp::RemoveRedundantMatsProcess().Execute()"
include_directories(${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/irrXML)	# Needed by "unrimp\External\RendererToolkit\Assimp\include\assimp\irrXMLWrapper.h"
include_directories(${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/unzip)	# Needed by "unrimp\External\RendererToolkit\Assimp\code\Common\ZipArchiveIOSystem.cpp"
include_directories(${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib)	# Needed by "unrimp\External\RendererToolkit\Assimp\contrib\unzip\unzip.h"
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})


##################################################
## Source codes
##################################################
set(EXTERNAL_SOURCE_CODES
	# crunch external library
	${CMAKE_SOURCE_DIR}/External/Renderer/crunch/crunchUnityBuild1.cpp
	${CMAKE_SOURCE_DIR}/External/Renderer/crunch/crunchUnityBuild2.cpp
	# MikkTSpace external library
	${CMAKE_SOURCE_DIR}/External/Renderer/MikkTSpace/mikktspace.c
	# Assimp external library
	${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/AssimpUnityBuild1.cpp
	${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/AssimpUnityBuild2.cpp
	${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/AssimpUnityBuild3.cpp
	${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/AssimpUnityBuild4.cpp
	${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/AssimpUnityBuild5.cpp
		# contrib
			# zlib (at least FBX importer is using it)
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/adler32.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/compress.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/crc32.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/deflate.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/gzclose.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/gzlib.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/gzread.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/gzwrite.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/infback.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/inffast.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/inflate.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/inftrees.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/trees.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/uncompr.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/zlib/zutil.c
			# unzip
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/unzip/ioapi.c
			${CMAKE_SOURCE_DIR}/External/RendererToolkit/Assimp/contrib/unzip/unzip.c
	# meshoptimizer external library
	${CMAKE_SOURCE_DIR}/External/RendererToolkit/meshoptimizer/MeshOptimizerUnityBuild.cpp
	# SimpleFileWatcher external library
	${CMAKE_SOURCE_DIR}/External/RendererToolkit/SimpleFileWatcher/FileWatcher.cpp
	# ies external library
	${CMAKE_SOURCE_DIR}/External/RendererToolkit/ies/ies_loader.cpp
)
if(WIN32)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		# SimpleFileWatcher external library
		${CMAKE_SOURCE_DIR}/External/RendererToolkit/SimpleFileWatcher/FileWatcherWin32.cpp
	)
elseif(UNIX)
	set(EXTERNAL_SOURCE_CODES
		${EXTERNAL_SOURCE_CODES}
		# SimpleFileWatcher external library
		${CMAKE_SOURCE_DIR}/External/RendererToolkit/SimpleFileWatcher/FileWatcherLinux.cpp
	)
endif()
set(SOURCE_CODES
	${EXTERNAL_SOURCE_CODES}
	Private/AssetCompiler/CompositorNodeAssetCompiler.cpp
	Private/AssetCompiler/CompositorWorkspaceAssetCompiler.cpp
	Private/AssetCompiler/MaterialAssetCompiler.cpp
	Private/AssetCompiler/MaterialBlueprintAssetCompiler.cpp
	Private/AssetCompiler/MeshAssetCompiler.cpp
	Private/AssetCompiler/SceneAssetCompiler.cpp
	Private/AssetCompiler/ShaderBlueprintAssetCompiler.cpp
	Private/AssetCompiler/ShaderPieceAssetCompiler.cpp
	Private/AssetCompiler/SkeletonAssetCompiler.cpp
	Private/AssetCompiler/VertexAttributesAssetCompiler.cpp
	Private/AssetCompiler/SkeletonAnimationAssetCompiler.cpp
	Private/AssetCompiler/TextureAssetCompiler.cpp
	Private/AssetImporter/SketchfabAssetImporter.cpp
	Private/Helper/JsonHelper.cpp
	Private/Helper/JsonMaterialBlueprintHelper.cpp
	Private/Helper/JsonMaterialHelper.cpp
	Private/Helper/StringHelper.cpp
	Private/Helper/AssimpHelper.cpp
	Private/Helper/AssimpIOSystem.cpp
	Private/Helper/AssimpLogStream.cpp
	Private/Helper/CacheManager.cpp
	Private/Project/ProjectAssetMonitor.cpp
	Private/Project/ProjectImpl.cpp
	Private/RendererToolkitImpl.cpp
)

# Add a natvis file for better debug support in Visual Studio ( https://docs.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects?view=vs-2019 )
if(MSVC)
	set(SOURCE_CODES ${SOURCE_CODES} ../Rhi/Rhi.natvis ../Renderer/Renderer.natvis)
endif(MSVC)

# Disable warnings in external libraries, we can't fix them
if(WIN32)
	# Used options:
	# - Disable "warning C4018: '<': signed/unsigned mismatch"
	# - Disable "warning C4100: 'data': unreferenced formal parameter"
	# - Disable "warning C4127: conditional expression is constant"
	# - Disable "warning C4131: 'adler32_z': uses old-style declarator"
	# - Disable "warning C4201: nonstandard extension used: nameless struct/union"
	# - Disable "warning C4204: nonstandard extension used: non-constant aggregate initializer"
	# - Disable "warning C4242: '=': conversion from 'unsigned int' to 'Bytef', possible loss of data"
	# - Disable "warning C4244: '=': conversion from 'unsigned int' to 'Bytef', possible loss of data"
	# - Disable "warning C4245: '=': conversion from 'int' to 'crnlib::uint', signed/unsigned mismatch"
	# - Disable "warning C4255: 'get_crc_table': no function prototype given: converting '()' to '(void)'"
	# - Disable "warning C4312: 'type cast': conversion from 'int' to 'unsigned char *' of greater size"
	# - Disable "warning C4365: 'argument': conversion from 'std::_Atomic_integral_t' to 'long', signed/unsigned mismatch"
	# - Disable "warning C4456: declaration of 'i' hides previous local declaration"
	# - Disable "warning C4457: declaration of 'm' hides function parameter"
	# - Disable "warning C4458: declaration of 'c' hides class member"
	# - Disable "warning C4464: relative include path contains '..'"
	# - Disable "warning C4548: expression before comma has no effect; expected expression with side-effect"
	# - Disable "warning C4554: '<<': check operator precedence for possible error; use parentheses to clarify precedence"
	# - Disable "warning C4555: result of expression not used"
	# - Disable "warning C4701: potentially uninitialized local variable 'refinerResults' used"
	# - Disable "warning C4774: '_scprintf' : format string expected in argument 1 is not a string literal""
	# - Disable "warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught"
	# - Disable "warning C4574: '_SECURE_SCL' is defined to be '0': did you mean to use '#if _SECURE_SCL'?"
	# - Disable "warning C4061: enumerator 'crnlib::PIXEL_FMT_DXT1' in switch of enum 'crnlib::pixel_format' is not explicitly handled by a case label"
	# - Disable "warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted"
	# - Disable "warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted"
	# - Disable "warning C4668: 'CRNLIB_SUPPORT_ETC_A1' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'"
	# - Disable "warning C4703: potentially uninitialized local pointer variable 'pScan_line' used"
	# - Disable "warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted"
	# - Disable "warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted"
	# - Disable "warning C5031: #pragma warning(pop): likely mismatch, popping warning state pushed in different file"
	# - Disable "warning C5032: detected #pragma warning(push) with no corresponding #pragma warning(pop)"
	# - Disable "warning C5038: data member 'crnlib::dxt_hc::m_has_etc_color_blocks' will be initialized after data member 'crnlib::dxt_hc::m_num_alpha_blocks'"
	# - Disable "warning C5039: 'ReadDirectoryChangesW': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception."
	# - Disable "warning C5219: implicit conversion from 'int' to 'float', possible loss of data"
	# - Disable "warning C5246 '<x>': the initialization of a subobject should be wrapped in braces"
	set_source_files_properties(${EXTERNAL_SOURCE_CODES} PROPERTIES COMPILE_FLAGS "/wd4018 /wd4100 /wd4127 /wd4131 /wd4201 /wd4204 /wd4242 /wd4244 /wd4245 /wd4255 /wd4312 /wd4365 /wd4456 /wd4457 /wd4458 /wd4464 /wd4548 /wd4554 /wd4555 /wd4701 /wd4774 /wd4571 /wd4574 /wd4061 /wd4625 /wd4626 /wd4703 /wd4668 /wd5026 /wd5027 /wd5031 /wd5032 /wd5038 /wd5039 /wd5219 /wd5246")
endif()


##################################################
## Libraries
##################################################
add_library(RendererToolkit ${LIBRARY_TYPE} ${SOURCE_CODES})
if(SHARED_LIBRARY)
	set_target_properties(RendererToolkit PROPERTIES COMPILE_FLAGS "-DSHARED_LIBRARIES -DRENDERER_TOOLKIT_EXPORTS")
	if(WIN32)
		target_link_libraries(RendererToolkit Renderer.lib)
	else()
		target_link_libraries(RendererToolkit Renderer)
		target_link_libraries(RendererToolkit dl)
	endif()
	add_dependencies(RendererToolkit Renderer)
else()
	if(UNIX)
		set(LIBRARIES ${LIBRARIES} stdc++fs dl)
	endif()
	target_link_libraries(RendererToolkit ${LIBRARIES})
	add_dependencies(RendererToolkit Renderer)
endif()
