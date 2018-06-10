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


// TODO(co) Migrate to "RENDERER_LOG()" to have a uniform log handling?


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "ConsoleExampleRunner.h"
#include "Framework/PlatformTypes.h"
#include "Framework/CommandLineArguments.h"

#include <iostream>


//[-------------------------------------------------------]
//[ Public virtual ExampleRunner methods                  ]
//[-------------------------------------------------------]
int ConsoleExampleRunner::run(const CommandLineArguments& commandLineArguments)
{
	if (!parseArgs(commandLineArguments))
	{
		printUsage(mAvailableExamples, mAvailableRenderers);
		return -1;
	}

	// Run example and switch as long between examples as asked to
	int result = 0;
	do
	{
		// Run current example
		result = runExample(mCurrentRendererName, mCurrentExampleName);
		if (0 == result && !mNextRendererName.empty() && !mNextExampleName.empty())
		{
			// Switch to next example
			mCurrentRendererName = mNextRendererName;
			mCurrentExampleName = mNextExampleName;
			mNextRendererName.clear();
			mNextExampleName.clear();
			result = 1;
		}
	} while (result);

	// Done
	return result;
}


//[-------------------------------------------------------]
//[ Protected virtual ExampleRunner methods               ]
//[-------------------------------------------------------]
void ConsoleExampleRunner::printUsage(const AvailableExamples& availableExamples, const AvailableRenderers& availableRenderers)
{
	std::cout << "Usage: ./Examples <exampleName> [-r <rendererName>]\n";

	// Available examples
	std::cout << "Available Examples:\n";
	for (const auto& pair : availableExamples)
	{
		std::cout << "\t" << pair.first << '\n';
	}

	// Available renderers
	std::cout << "Available Renderer:\n";
	for (const std::string& rendererName : availableRenderers)
	{
		std::cout << "\t" << rendererName << '\n';
	}
}

void ConsoleExampleRunner::showError(const std::string& errorMessage)
{
	std::cout << errorMessage << "\n";
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
bool ConsoleExampleRunner::parseArgs(const CommandLineArguments& commandLineArguments)
{
	uint32_t numberOfArguments = commandLineArguments.getCount();
	for (uint32_t argumentIndex = 0; argumentIndex < numberOfArguments; ++argumentIndex)
	{
		const std::string argument = commandLineArguments.getArgumentAtIndex(argumentIndex);
		if (argument != "-r")
		{
			mCurrentExampleName = argument;
		}
		else if (argumentIndex + 1 < numberOfArguments)
		{
			++argumentIndex;
			mCurrentRendererName = commandLineArguments.getArgumentAtIndex(argumentIndex);
		}
		else
		{
			showError("Missing argument for parameter -r");

			// Error!
			return false;
		}
	}

	if (mCurrentRendererName.empty())
	{
		mCurrentRendererName = mDefaultRendererName;
	}

	// Done
	return true;
}
