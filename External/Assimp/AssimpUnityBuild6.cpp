/*********************************************************\
 * Copyright (c) 2012-2019 The Unrimp Team
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
	__pragma(warning(disable: 4061))	// warning C4061: enumerator 'FORCE_32BIT' in switch of enum 'aiMetadataType' is not explicitly handled by a case label
	__pragma(warning(disable: 4065))	// warning C4065: switch statement contains 'default' but no 'case' labels
	__pragma(warning(disable: 4100))	// warning C4100: 'r': unreferenced formal parameter
	__pragma(warning(disable: 4127))	// warning C4127: conditional expression is constant
	__pragma(warning(disable: 4189))	// warning C4189: 'maxIndexRequested': local variable is initialized but not referenced
	__pragma(warning(disable: 4242))	// warning C4242: '=': conversion from 'int' to 'char', possible loss of data
	__pragma(warning(disable: 4244))	// warning C4244: '=': conversion from 'int' to 'char', possible loss of data
	__pragma(warning(disable: 4245))	// warning C4245: 'argument': conversion from 'const long' to '::size_t', signed/unsigned mismatch
	__pragma(warning(disable: 4310))	// warning C4310: cast truncates constant value
	__pragma(warning(disable: 4355))	// warning C4355: 'this': used in base member initializer list
	__pragma(warning(disable: 4365))	// warning C4365: '=': conversion from 'int' to '::size_t', signed/unsigned mismatch
	__pragma(warning(disable: 4389))	// warning C4389: '==': signed/unsigned mismatch
	__pragma(warning(disable: 4456))	// warning C4456: declaration of 'pv2' hides previous local declaration
	__pragma(warning(disable: 4457))	// warning C4457: declaration of 'it' hides function parameter
	__pragma(warning(disable: 4458))	// warning C4458: declaration of 'sep' hides class member
	__pragma(warning(disable: 4459))	// warning C4459: declaration of 'BufferSize' hides global declaration
	__pragma(warning(disable: 4464))	// warning C4464: relative include path contains '..'
	__pragma(warning(disable: 4505))	// warning C4505: 'Assimp::FBX::validateAnimCurveNodes': unreferenced local function has been removed
	__pragma(warning(disable: 4541))	// warning C4541: 'dynamic_cast' used on polymorphic type 'Assimp::FBX::Object' with /GR-; unpredictable behavior may result
	__pragma(warning(disable: 4571))	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	__pragma(warning(disable: 4574))	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	__pragma(warning(disable: 4619))	// warning C4619: #pragma warning: there is no warning number '4351'
	__pragma(warning(disable: 4623))	// warning C4623: 'std::_List_node<_Ty,std::_Default_allocator_traits<_Alloc>::void_pointer>': default constructor was implicitly defined as deleted
	__pragma(warning(disable: 4625))	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	__pragma(warning(disable: 4626))	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 4668))	// warning C4668: '__ANDROID__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	__pragma(warning(disable: 4701))	// warning C4701: potentially uninitialized local variable 'posEpsilon' used
	__pragma(warning(disable: 4702))	// warning C4702: unreachable code
	__pragma(warning(disable: 4706))	// warning C4706: assignment within conditional expression
	__pragma(warning(disable: 4714))	// warning C4714: function 'bool __cdecl Assimp::Ogre::EndsWith(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const & __ptr64,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const & __ptr64,bool)' marked as __forceinline not inlined
	__pragma(warning(disable: 4774))	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	__pragma(warning(disable: 4826))	// warning C4826: Conversion from 'aiBone **' to 'uint64_t' is sign-extended. This may cause unexpected runtime behavior. (compiling source file ..\..\External\Assimp\AssimpUnityBuild1.cpp)
	__pragma(warning(disable: 5026))	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	__pragma(warning(disable: 5027))	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 5039))	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
#endif
// COLLADA
	#include "code/ColladaParser.cpp"
// OGRE
	#include "code/OgreImporter.cpp"
	#include "code/OgreMaterial.cpp"
