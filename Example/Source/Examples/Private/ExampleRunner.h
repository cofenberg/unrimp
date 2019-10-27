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


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Examples/Private/Framework/PlatformTypes.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4574)	// warning C4574: '_HAS_ITERATOR_DEBUGGING' is defined to be '0': did you mean to use '#if _HAS_ITERATOR_DEBUGGING'?
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	#include <map>
	#include <set>
	#include <vector>
	#include <string>
	#include <string_view>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class CommandLineArguments;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
class ExampleRunner final
{


//[-------------------------------------------------------]
//[ Public definitions                                    ]
//[-------------------------------------------------------]
public:
	typedef int(*RunnerMethod)(ExampleRunner&, const char*);
	typedef std::map<std::string_view, RunnerMethod>  AvailableExamples;
	typedef std::set<std::string_view>				  AvailableRhis;
	typedef std::vector<std::string_view>			  SupportedRhis;
	typedef std::map<std::string_view, SupportedRhis> ExampleToSupportedRhis;


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	ExampleRunner();

	inline ~ExampleRunner()
	{}

	[[nodiscard]] inline const AvailableRhis& getAvailableRhis() const
	{
		return mAvailableRhis;
	}

	[[nodiscard]] inline const ExampleToSupportedRhis& getExampleToSupportedRhis() const
	{
		return mExampleToSupportedRhis;
	}

	[[nodiscard]] inline const std::string& getDefaultRhiName() const
	{
		return mDefaultRhiName;
	}

	[[nodiscard]] inline const std::string& getDefaultExampleName() const
	{
		return mDefaultExampleName;
	}

	[[nodiscard]] inline const std::string& getCurrentRhiName() const
	{
		return mCurrentRhiName;
	}

	[[nodiscard]] inline const std::string& getCurrentExampleName() const
	{
		return mCurrentExampleName;
	}

	[[nodiscard]] int run(const CommandLineArguments& commandLineArguments);

	/**
	*  @brief
	*    Ask the example runner politely to switch to another example as soon as possible
	*
	*  @param[in] exampleName
	*    Example name, must be valid
	*  @param[in] rhiName
	*    RHI name, if null pointer the default RHI will be used
	*/
	void switchExample(const char* exampleName, const char* rhiName = nullptr);


//[-------------------------------------------------------]
//[ Private static methods                                ]
//[-------------------------------------------------------]
private:
	template <class ExampleClass>
	[[nodiscard]] static int runRenderExample(ExampleRunner& exampleRunner, const char* rhiName)
	{
		ExampleClass exampleClass;
		exampleClass.mExampleRunner = &exampleRunner;
		return IApplicationRhi(rhiName, exampleClass).run();
	}

	template <class ExampleClass>
	[[nodiscard]] static int runRenderRuntimeExample(ExampleRunner& exampleRunner, const char* rhiName)
	{
		ExampleClass exampleClass;
		exampleClass.mExampleRunner = &exampleRunner;
		return IApplicationRendererRuntime(rhiName, exampleClass).run();
	}

	template <class ExampleClass>
	[[nodiscard]] static int runBasicExample(ExampleRunner& exampleRunner, const char* rhiName)
	{
		ExampleClass exampleClass(exampleRunner, rhiName);
		return exampleClass.run();
	}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	[[nodiscard]] bool parseCommandLineArguments(const CommandLineArguments& commandLineArguments);
	void printUsage(const AvailableExamples& availableExamples, const AvailableRhis& availableRhis);
	void showError(const std::string& errorMessage);
	[[nodiscard]] int runExample(const std::string_view& rhiName, const std::string_view& exampleName);

	template<typename T>
	void addExample(const std::string_view& name, RunnerMethod runnerMethod, const T& supportedRhiList)
	{
		mAvailableExamples.insert(std::pair<std::string_view, RunnerMethod>(name, runnerMethod));
		SupportedRhis supportedRhis;
		for (const std::string_view& supportedRhi : supportedRhiList)
		{
			supportedRhis.push_back(supportedRhi);
		}
		mExampleToSupportedRhis.insert(std::pair<std::string_view, SupportedRhis>(name, std::move(supportedRhis)));
	}


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	AvailableExamples 	   mAvailableExamples;
	AvailableRhis		   mAvailableRhis;
	ExampleToSupportedRhis mExampleToSupportedRhis;
	std::string			   mDefaultRhiName;
	std::string			   mDefaultExampleName;
	std::string			   mCurrentRhiName;
	std::string			   mCurrentExampleName;
	std::string			   mNextRhiName;
	std::string			   mNextExampleName;


};
