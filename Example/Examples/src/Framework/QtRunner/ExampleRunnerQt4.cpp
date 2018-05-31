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
#include "Framework/PlatformTypes.h"
#include "Framework/CommandLineArguments.h"
#include "Framework/QtRunner/CLocaleChanger.h"
#include "Framework/QtRunner/ExampleRunnerQt4.h"

#include <QMessageBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QHBoxLayout>
#include <QApplication>

#include <iostream>


void ExampleRunnerQt4::showError(const std::string errorMsg)
{
	QMessageBox::critical(nullptr, "Error", errorMsg.c_str());
}

void ExampleRunnerQt4::printUsage(const ExampleRunner::AvailableExamplesMap& knownExamples, const ExampleRunner::AvailableRendererMap& availableRenderer)
{
	std::cout<<"Usage: ./Examples <exampleName> [-r <rendererName>]\n";
	std::cout<<"Available Examples:\n";
	for (AvailableExamplesMap::const_iterator it=knownExamples.cbegin(); it!=knownExamples.cend(); ++it)
		std::cout <<"\t"<< it->first<< '\n';
	std::cout<<"Available Renderer:\n";
	for (AvailableRendererMap::const_iterator it=availableRenderer.cbegin(); it!=availableRenderer.cend(); ++it)
		std::cout <<"\t"<< *it<< '\n';
}

int ExampleRunnerQt4::run(const CommandLineArguments& commandLineArguments)
{
	int nu = 0;
	QApplication app(nu, nullptr);
	
	// activate the 'C' locale
	CLocaleChanger cLocale;
	
	if(!parseArgs(commandLineArguments)) {
		printUsage(m_availableExamples, m_availableRenderer);
		return -1;
	}
	
	QDialog dialog;
	dialog.setWindowTitle("Unrimp simple Example launcher (Qt4)");
	dialog.setLayout(new QHBoxLayout(&dialog));
	QLabel *label = new QLabel("renderer:", &dialog);
	dialog.layout()->addWidget(label);
	QComboBox *rendererBox = new QComboBox(&dialog);
	for (AvailableRendererMap::const_iterator it=m_availableRenderer.cbegin(); it!=m_availableRenderer.cend(); ++it)
		rendererBox->addItem((*it).c_str());
	dialog.layout()->addWidget(rendererBox);
	if(!m_rendererName.empty()) {
		int index = rendererBox->findText(m_rendererName.c_str());
		if ( index != -1 ) { // -1 for not found
			rendererBox->setCurrentIndex(index);
		}
	}
	
	label = new QLabel("Examples:", &dialog);
	dialog.layout()->addWidget(label);
	QComboBox *box = new QComboBox(&dialog);
	for (AvailableExamplesMap::const_iterator it=m_availableExamples.cbegin(); it!=m_availableExamples.cend(); ++it)
		box->addItem(it->first.c_str());
	dialog.layout()->addWidget(box);
	if(!m_exampleName.empty()) {
		int index = box->findText(m_exampleName.c_str());
		if ( index != -1 ) { // -1 for not found
			box->setCurrentIndex(index);
		}
	}
	
	QPushButton *button = new QPushButton("Ok", &dialog);
	button->connect(button, SIGNAL(clicked()), &dialog, SLOT(accept()));
	dialog.layout()->addWidget(button);
	
	
	int result = dialog.exec();
	if (result == 1) {
		m_rendererName = rendererBox->currentText().toUtf8().data();
		m_exampleName = box->currentText().toUtf8().data();
		result = runExample(m_rendererName, m_exampleName);
	}
	else
	{
		result = -1;
	}
	return result;
}

bool ExampleRunnerQt4::parseArgs(const CommandLineArguments& commandLineArguments) {
	unsigned int length = commandLineArguments.getCount();
	for(unsigned int i = 0; i < length; ++i) {
		std::string arg = commandLineArguments.getArgumentAtIndex(i);
		if (arg != "-r") {
			m_exampleName = arg;
		}
		else {
			if (i+1 < length) {
				++i;
				m_rendererName = commandLineArguments.getArgumentAtIndex(i);
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
