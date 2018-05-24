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

#pragma once


#include <map>
#include <set>
#include <string>
#include <vector>


class CommandLineArguments;

class ExampleRunner
{
public:
	virtual int run(const CommandLineArguments &args) = 0;
	virtual ~ExampleRunner();

protected:
	typedef int(*RunnerMethod)(const char*);
	typedef std::map<std::string, RunnerMethod> AvailableExamplesMap;
	typedef std::set<std::string> AvailableRendererMap;
	typedef std::map<std::string, std::vector<std::string>> ExampleToSupportedRendererMap;
	
	ExampleRunner();
	
	virtual void printUsage(const AvailableExamplesMap &knownExamples, const AvailableRendererMap &availableRenderer) = 0;
	virtual void showError(const std::string& errorMessage) = 0;
	
	int runExample(const std::string &rendererName, const std::string &exampleName);

private:
	template<typename T>
	void addExample(const std::string& name, RunnerMethod runnerMethod, T const &supportedRendererList);

protected:
	AvailableExamplesMap 			m_availableExamples;
	AvailableRendererMap 			m_availableRenderer;
	ExampleToSupportedRendererMap	m_supportedRendererForExample;
	std::string						m_defaultRendererName;
	std::string						m_defaultExampleName;
};
