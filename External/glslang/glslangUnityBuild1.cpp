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


// Amalgamated/unity build
#ifdef WIN32
	// Disable warnings in external headers, we can't fix them
	__pragma(warning(disable: 4061))	// warning C4061: enumerator 'EBadProfile' in switch of enum 'EProfile' is not explicitly handled by a case label
	__pragma(warning(disable: 4100))	// warning C4100: 'typeProxy': unreferenced formal parameter
	__pragma(warning(disable: 4189))	// warning C4189: 'isFloat': local variable is initialized but not referenced (compiling source file ..\..\..\External\glslang\glslangUnityBuild1.cpp)
	__pragma(warning(disable: 4242))	// warning C4242: '=': conversion from 'int' to 'yytype_int16', possible loss of data
	__pragma(warning(disable: 4244))	// warning C4244: '=': conversion from 'int' to 'char', possible loss of data
	__pragma(warning(disable: 4266))	// warning C4266: 'bool TLinker::link(THandleList &)': no override available for virtual member function from base 'TLinker'; function is hidden
	__pragma(warning(disable: 4310))	// warning C4310: cast truncates constant value (compiling source file ..\..\..\External\glslang\glslangUnityBuild1.cpp)
	__pragma(warning(disable: 4365))	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	__pragma(warning(disable: 4456))	// warning C4456: declaration of 'feature' hides previous local declaration
	__pragma(warning(disable: 4457))	// warning C4457: declaration of 'token' hides function parameter
	__pragma(warning(disable: 4458))	// warning C4458: declaration of 'name' hides class member
	__pragma(warning(disable: 4459))	// warning C4459: declaration of 'tokens' hides global declaration
	__pragma(warning(disable: 4464))	// warning C4464: relative include path contains '..'
	__pragma(warning(disable: 4530))	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	__pragma(warning(disable: 4571))	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	__pragma(warning(disable: 4574))	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	__pragma(warning(disable: 4623))	// warning C4623: 'std::_List_node<_Ty,std::_Default_allocator_traits<_Alloc>::void_pointer>': default constructor was implicitly defined as deleted
	__pragma(warning(disable: 4625))	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	__pragma(warning(disable: 4626))	// warning C4626: 'std::codecvt_base': assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 4668))	// warning C4668: '_WIN32_WINNT_WIN10_TH2' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	__pragma(warning(disable: 4702))	// warning C4702: unreachable code
	__pragma(warning(disable: 4774))	// warning C4774: 'snprintf' : format string expected in argument 3 is not a string literal
	__pragma(warning(disable: 4946))	// warning C4946: reinterpret_cast used between related classes: 'TIntermNode' and 'glslang::TIntermTyped'
	__pragma(warning(disable: 5026))	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	__pragma(warning(disable: 5027))	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	__pragma(warning(disable: 5039))	// warning C5039: 'TpSetCallbackCleanupGroup': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
#endif
#include "glslang/GenericCodeGen/CodeGen.cpp"
#include "glslang/GenericCodeGen/Link.cpp"
#include "glslang/MachineIndependent/preprocessor/Pp.cpp"
#include "glslang/MachineIndependent/preprocessor/PpAtom.cpp"
#include "glslang/MachineIndependent/preprocessor/PpContext.cpp"
#include "glslang/MachineIndependent/preprocessor/PpScanner.cpp"
#include "glslang/MachineIndependent/preprocessor/PpTokens.cpp"
#include "glslang/MachineIndependent/attribute.cpp"
#include "glslang/MachineIndependent/Constant.cpp"
#include "glslang/MachineIndependent/glslang_tab.cpp"
#include "glslang/MachineIndependent/InfoSink.cpp"
#include "glslang/MachineIndependent/Initialize.cpp"
#include "glslang/MachineIndependent/Intermediate.cpp"
#include "glslang/MachineIndependent/intermOut.cpp"
#include "glslang/MachineIndependent/IntermTraverse.cpp"
#include "glslang/MachineIndependent/iomapper.cpp"
#include "glslang/MachineIndependent/limits.cpp"
#include "glslang/MachineIndependent/linkValidate.cpp"
#include "glslang/MachineIndependent/ParseContextBase.cpp"
#include "glslang/MachineIndependent/ParseHelper.cpp"
#include "glslang/MachineIndependent/PoolAlloc.cpp"
#include "glslang/MachineIndependent/propagateNoContraction.cpp"
#include "glslang/MachineIndependent/reflection.cpp"
#include "glslang/MachineIndependent/RemoveTree.cpp"
//#include "glslang/MachineIndependent/Scan.cpp"		// Moved into "glslangUnityBuild2.cpp" since it doesn't play well together with the other source codes
//#include "glslang/MachineIndependent/ShaderLang.cpp"	// Moved into "glslangUnityBuild2.cpp" since it doesn't play well together with the other source codes
#include "glslang/MachineIndependent/SymbolTable.cpp"
#include "glslang/MachineIndependent/Versions.cpp"
#include "glslang/MachineIndependent/parseConst.cpp"
#include "OGLCompilersDLL/InitializeDll.cpp"
#include "SPIRV/GlslangToSpv.cpp"
#include "SPIRV/InReadableOrder.cpp"
#include "SPIRV/Logger.cpp"
#include "SPIRV/SpvBuilder.cpp"

// Moved into "glslangUnityBuild3.cpp" since it doesn't play well together with the other source codes
#ifdef WIN32
	// #include "glslang/OSDependent/Windows/ossource.cpp"
#elif UNIX
	// #include "glslang/OSDependent/Unix/ossource.cpp"
#endif
