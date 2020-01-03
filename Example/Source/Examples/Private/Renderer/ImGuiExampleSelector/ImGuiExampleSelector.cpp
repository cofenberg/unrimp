/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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


#ifndef RENDERER_IMGUI
	#error "This file only works when the renderer is build with ImGui support enabled"
#endif


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Examples/Private/Renderer/ImGuiExampleSelector/ImGuiExampleSelector.h"
#include "Examples/Private/Framework/Color4.h"
#include "Examples/Private/ExampleRunner.h"

#include <Renderer/Public/IRenderer.h>
#include <Renderer/Public/DebugGui/DebugGuiManager.h>
#ifdef RENDERER_OPENVR
	#include <Renderer/Public/Vr/IVrManager.h>
#endif

#include <ImGui/imgui.h>


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void ImGuiExampleSelector::onInitialization()
{
	// Ease-of-use: If a HMD is present automatically start the "Scene"-example so the user can see something
	#ifdef RENDERER_OPENVR
		if (getRendererSafe().getVrManager().isHmdPresent())
		{
			switchExample("Scene");
		}
	#endif
}

void ImGuiExampleSelector::onDraw()
{
	// Get and check the RHI instance
	Rhi::IRhiPtr rhi(getRhi());
	if (nullptr != rhi)
	{
		// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
		Rhi::Command::ClearGraphics::create(mCommandBuffer, Rhi::ClearFlag::COLOR_DEPTH, Color4::GRAY);

		// GUI
		if (nullptr != getMainRenderTarget())
		{
			Renderer::DebugGuiManager& debugGuiManager = getRendererSafe().getDebugGuiManager();
			debugGuiManager.newFrame(*getMainRenderTarget());
			createDebugGui();
			debugGuiManager.fillGraphicsCommandBufferUsingFixedBuildInRhiConfiguration(mCommandBuffer);
		}

		// Submit command buffer to the RHI implementation
		mCommandBuffer.submitToRhiAndClear(*rhi);
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
		// Selection of RHI
		std::string selectedRhiName;
		static int selectedRhiIndex = -1;
		{
			// Fill list of RHI names
			std::string itemsSeparatedByZeros;
			const ExampleRunner::AvailableRhis& availableRhis = getExampleRunner().getAvailableRhis();
			{
				int rhiIndex = 0;
				const std::string& defaultRhiName = getExampleRunner().getDefaultRhiName();
				for (const std::string_view& rhiName : availableRhis)
				{
					itemsSeparatedByZeros += rhiName;
					itemsSeparatedByZeros += '\0';
					if (-1 == selectedRhiIndex && defaultRhiName == rhiName)
					{
						// Set initially selected RHI index
						selectedRhiIndex = rhiIndex;
					}
					if (rhiIndex == selectedRhiIndex)
					{
						selectedRhiName = rhiName;
					}
					++rhiIndex;
				}
				itemsSeparatedByZeros += '\0';
			}

			// Tell ImGui
			ImGui::Combo("RHI", &selectedRhiIndex, itemsSeparatedByZeros.c_str());
		}

		// Selection of example
		std::string selectedExampleName;
		{
			static int selectedExampleIndex = -1;

			// Fill list of examples supported by the currently selected RHI
			std::string itemsSeparatedByZeros;
			{
				int exampleIndex = 0;
				const ExampleRunner::ExampleToSupportedRhis& exampleToSupportedRhis = getExampleRunner().getExampleToSupportedRhis();
				for (const auto& pair : exampleToSupportedRhis)
				{
					const ExampleRunner::SupportedRhis& supportedRhiList = pair.second;
					if (pair.first != "ImGuiExampleSelector" && std::find(supportedRhiList.begin(), supportedRhiList.end(), selectedRhiName) != supportedRhiList.end())
					{
						itemsSeparatedByZeros += pair.first;
						itemsSeparatedByZeros += '\0';
						if (-1 == selectedExampleIndex && "Scene" == pair.first)
						{
							// Set initially selected example index, "Scene" is preferred since it's the most advantaged example
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

				// In case the default example isn't supported by the selected RHI, initially select the first best supported example
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
			switchExample(selectedExampleName.c_str(), selectedRhiName.c_str());
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
