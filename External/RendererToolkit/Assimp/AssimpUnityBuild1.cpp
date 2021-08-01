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


// Amalgamated/unity build
#ifdef _WIN32
	// Disable warnings in external headers, we can't fix them
	__pragma(warning(disable: 4355))	// warning C4355: 'this': used in base member initializer list
	__pragma(warning(disable: 4619))	// warning C4619: #pragma warning: there is no warning number '4351'
	__pragma(warning(disable: 4777))	// warning C4777: 'snprintf' : format string '%Iu' requires an argument of type 'unsigned int', but variadic argument 1 has type 'const size_t'
	__pragma(warning(disable: 4826))	// warning C4826: Conversion from 'aiBone **' to 'uint64_t' is sign-extended. This may cause unexpected runtime behavior.
#endif
#include "Configuration.h"
// code
	// AssetLib
		// Blender
		#include "code/AssetLib/Blender/BlenderBMesh.cpp"
		#include "code/AssetLib/Blender/BlenderCustomData.cpp"
		#include "code/AssetLib/Blender/BlenderDNA.cpp"
		#include "code/AssetLib/Blender/BlenderLoader.cpp"
		#include "code/AssetLib/Blender/BlenderModifier.cpp"
		//#include "code/AssetLib/Blender/BlenderScene.cpp"		# Due to conflict moved into "AssimpUnityBuild2.cpp"
		#include "code/AssetLib/Blender/BlenderTessellator.cpp"
		// Collada
		#include "code/AssetLib/Collada/ColladaHelper.cpp"
		//#include "code/AssetLib/Collada/ColladaLoader.cpp"	# Due to conflict moved into "AssimpUnityBuild2.cpp"
		//#include "code/AssetLib/Collada/ColladaParser.cpp"	# Due to conflict moved into "AssimpUnityBuild3.cpp"
		// FBX
		#include "code/AssetLib/FBX/FBXAnimation.cpp"
		#include "code/AssetLib/FBX/FBXBinaryTokenizer.cpp"
		#include "code/AssetLib/FBX/FBXConverter.cpp"
		#include "code/AssetLib/FBX/FBXDeformer.cpp"
		#include "code/AssetLib/FBX/FBXDocument.cpp"
		#include "code/AssetLib/FBX/FBXDocumentUtil.cpp"
		//#include "code/AssetLib/FBX/FBXImporter.cpp"		# Due to conflict moved into "AssimpUnityBuild5.cpp"
		#include "code/AssetLib/FBX/FBXMaterial.cpp"
		#include "code/AssetLib/FBX/FBXMeshGeometry.cpp"
		#include "code/AssetLib/FBX/FBXModel.cpp"
		#include "code/AssetLib/FBX/FBXNodeAttribute.cpp"
		//#include "code/AssetLib/FBX/FBXParser.cpp"	# Due to conflict moved into "AssimpUnityBuild5.cpp"
		#include "code/AssetLib/FBX/FBXProperties.cpp"
		#include "code/AssetLib/FBX/FBXTokenizer.cpp"
		#include "code/AssetLib/FBX/FBXUtil.cpp"
		// glTF
		#include "../RapidJSON/configuration/rapidjson/rapidjson.h"
		#include "code/AssetLib/glTF/glTFCommon.cpp"
		// glTF2
		//#include "code/AssetLib/glTF2/glTF2Importer.cpp"	# Due to conflict moved into "AssimpUnityBuild4.cpp"
		// MD5
		//#include "code/AssetLib/MD5/MD5Loader.cpp"	# Due to conflict moved into "AssimpUnityBuild2.cpp"
		#include "code/AssetLib/MD5/MD5Parser.cpp"
		// Obj
		//#include "code/AssetLib/Obj/ObjFileImporter.cpp"	# Due to conflict moved into "AssimpUnityBuild3.cpp"
		#include "code/AssetLib/Obj/ObjFileMtlImporter.cpp"
		#include "code/AssetLib/Obj/ObjFileParser.cpp"
	// CApi
	#include "code/CApi/AssimpCExport.cpp"
	#include "code/CApi/CInterfaceIOWrapper.cpp"
