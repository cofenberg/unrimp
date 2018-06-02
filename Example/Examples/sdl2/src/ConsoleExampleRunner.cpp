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
#include "ConsoleExampleRunner.h"
#include "Framework/PlatformTypes.h"
#include "Framework/CommandLineArguments.h"

#include <iostream>


void ConsoleExampleRunner::showError(const std::string& errorMessage)
{
	std::cout << errorMessage << "\n";
}

void ConsoleExampleRunner::printUsage(const ExampleRunner::AvailableExamplesMap& knownExamples, const ExampleRunner::AvailableRendererMap& availableRenderer)
{
	std::cout << "Usage: ./Examples <exampleName> [-r <rendererName>]\n";
	std::cout << "Available Examples:\n";
	for (AvailableExamplesMap::const_iterator it=knownExamples.cbegin(); it!=knownExamples.cend(); ++it)
		std::cout << "\t" << it->first << '\n';
	std::cout << "Available Renderer:\n";
	for (AvailableRendererMap::const_iterator it=availableRenderer.cbegin(); it!=availableRenderer.cend(); ++it)
		std::cout << "\t" << *it << '\n';
}

int ConsoleExampleRunner::run(const CommandLineArguments& args)
{
	if(!parseArgs(args)) {
		printUsage(m_availableExamples, m_availableRenderer);
		return -1;
	}
	return runExample(m_rendererName, m_exampleName);
}

bool ConsoleExampleRunner::parseArgs(const CommandLineArguments &args) {
	uint32_t length = args.getCount();
	for(unsigned int i = 0; i < length; ++i) {
		std::string arg = args.getArgumentAtIndex(i); 
		if (arg != "-r") {
			m_exampleName = arg;
		}
		else {
			if (i+1 < length) {
				++i;
				m_rendererName = args.getArgumentAtIndex(i);
			}
			else
			{
				showError("missing argument for parameter -r");
				return false;
			}
		}
		
	}

	if (m_rendererName.empty())
		m_rendererName = m_defaultRendererName;

	return true;
}
