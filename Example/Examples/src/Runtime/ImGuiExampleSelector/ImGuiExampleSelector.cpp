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


#ifndef RENDERER_RUNTIME_IMGUI
	#error "This file only works when the renderer runtime is build with ImGui support enabled"
#endif


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Runtime/ImGuiExampleSelector/ImGuiExampleSelector.h"
#include "Framework/Color4.h"
#include "ExampleRunner.h"

#include <RendererRuntime/IRendererRuntime.h>
#include <RendererRuntime/DebugGui/DebugGuiManager.h>

#include <imgui/imgui.h>


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void ImGuiExampleSelector::onDraw()
{
	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Clear the color buffer of the current render target with gray, do also clear the depth buffer
		Renderer::Command::Clear::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

		{ // GUI
			RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
			if (nullptr != rendererRuntime && nullptr != getMainRenderTarget())
			{
				RendererRuntime::DebugGuiManager& debugGuiManager = rendererRuntime->getDebugGuiManager();
				debugGuiManager.newFrame(*getMainRenderTarget());
				createDebugGui();
				debugGuiManager.fillCommandBufferUsingFixedBuildInRendererConfiguration(mCommandBuffer);
			}
		}

		// Submit command buffer to the renderer backend
		mCommandBuffer.submitToRendererAndClear(*renderer);
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void ImGuiExampleSelector::createDebugGui()
{
	ImGui::SetNextWindowSize(ImVec2(260.0f, 100.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Example Selector"))
	{
		// Selection of renderer
		std::string selectedRendererName;
		static int selectedRendererIndex = -1;
		{
			// Fill list of renderer names
			std::string itemsSeparatedByZeros;
			const ExampleRunner::AvailableRenderers& availableRenderers = getExampleRunner().getAvailableRenderers();
			{
				int rendererIndex = 0;
				const std::string& defaultRendererName = getExampleRunner().getDefaultRendererName();
				for (const std::string& rendererName : availableRenderers)
				{
					itemsSeparatedByZeros += rendererName;
					itemsSeparatedByZeros += '\0';
					if (-1 == selectedRendererIndex && defaultRendererName == rendererName)
					{
						// Set initially selected renderer index
						selectedRendererIndex = rendererIndex;
					}
					if (rendererIndex == selectedRendererIndex)
					{
						selectedRendererName = rendererName;
					}
					++rendererIndex;
				}
				itemsSeparatedByZeros += '\0';
			}

			// Tell ImGui
			ImGui::Combo("Renderer", &selectedRendererIndex, itemsSeparatedByZeros.c_str());
		}

		// Selection of example
		std::string selectedExampleName;
		{
			static int selectedExampleIndex = -1;

			// Fill list of examples supported by the currently selected renderer
			std::string itemsSeparatedByZeros;
			{
				int exampleIndex = 0;
				const ExampleRunner::ExampleToSupportedRenderers& exampleToSupportedRenderers = getExampleRunner().getExampleToSupportedRenderers();
				for (const auto& pair : exampleToSupportedRenderers)
				{
					const ExampleRunner::SupportedRenderers& supportedRendererList = pair.second;
					if (pair.first != "ImGuiExampleSelector" && std::find(supportedRendererList.begin(), supportedRendererList.end(), selectedRendererName) != supportedRendererList.end())
					{
						itemsSeparatedByZeros += pair.first;
						itemsSeparatedByZeros += '\0';
						if (-1 == selectedExampleIndex && "FirstScene" == pair.first)
						{
							// Set initially selected example index, "FirstScene" is preferred since it's the most advantaged example
							selectedExampleIndex = exampleIndex;
						}
						if (exampleIndex == selectedExampleIndex)
						{
							selectedExampleName = pair.first;
						}
						++exampleIndex;
					}
				}
				itemsSeparatedByZeros += '\0';

				// In case the default example isn't supported by the selected renderer, initially select the first best supported example
				if (-1 == selectedExampleIndex && 0 != exampleIndex)
				{
					selectedExampleIndex = 0;
					selectedExampleName = itemsSeparatedByZeros;
				}
			}

			// Tell ImGui
			ImGui::Combo("Example", &selectedExampleIndex, itemsSeparatedByZeros.c_str());
		}

		// Start selected example
		if (ImGui::Button("Start"))
		{
			switchExample(selectedExampleName.c_str(), selectedRendererName.c_str());
		}

		// Exit
		ImGui::SameLine();
		if (ImGui::Button("Exit"))
		{
			exit();
		}
	}
	ImGui::End();
}
