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

#include <Renderer/Public/Context.h>
#include <Renderer/Public/IRenderer.h>
#include <Renderer/Public/Core/File/IFile.h>
#include <Renderer/Public/Core/File/IFileManager.h>
#include <Renderer/Public/DebugGui/DebugGuiManager.h>
#ifdef RENDERER_OPENVR
	#include <Renderer/Public/Vr/IVrManager.h>
#endif

// "ini.h"-library
#define INI_MALLOC(ctx, size) (static_cast<Rhi::IAllocator*>(ctx)->reallocate(nullptr, 0, size, 1))
#define INI_FREE(ctx, ptr) (static_cast<Rhi::IAllocator*>(ctx)->reallocate(ptr, 0, 0, 1))
#include <ini/ini.h>

#include <ImGui/imgui.h>


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace ImGuiExampleSelectorDetail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr char* VIRTUAL_SETTINGS_FILENAME = "LocalData/ImGuiExampleSelectorExample.ini";


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
ImGuiExampleSelector::ImGuiExampleSelector() :
	// Ini settings indices
	mIni(nullptr),
	mSelectedRhiNameIndex(INI_NOT_FOUND),
	mSelectedExampleNameIndex(INI_NOT_FOUND)
{
	// Nothing here
}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void ImGuiExampleSelector::onInitialization()
{
	loadIni();

	// Ease-of-use: If a HMD is present automatically start the "Scene"-example so the user can see something
	#ifdef RENDERER_OPENVR
		if (getRendererSafe().getVrManager().isHmdPresent())
		{
			switchExample("Scene");
		}
	#endif
}

void ImGuiExampleSelector::onDeinitialization()
{
	saveIni();
	destroyIni();
}

void ImGuiExampleSelector::onDraw(Rhi::CommandBuffer& commandBuffer)
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

	// Append command buffer to the given command buffer
	mCommandBuffer.appendToCommandBufferAndClear(commandBuffer);
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void ImGuiExampleSelector::loadIni()
{
	// Reset ini
	destroyIni();

	// Try to load ini settings from file
	Renderer::IRenderer& renderer = getRendererSafe();
	const Renderer::IFileManager& fileManager = renderer.getFileManager();
	if (fileManager.doesFileExist(::ImGuiExampleSelectorDetail::VIRTUAL_SETTINGS_FILENAME))
	{
		Renderer::IFile* file = fileManager.openFile(Renderer::IFileManager::FileMode::READ, ::ImGuiExampleSelectorDetail::VIRTUAL_SETTINGS_FILENAME);
		if (nullptr != file)
		{
			mIniFileContent.resize(file->getNumberOfBytes());
			file->read(mIniFileContent.data(), mIniFileContent.size());
			fileManager.closeFile(*file);
			mIni = ini_load(mIniFileContent.data(), &renderer.getContext().getAllocator());
			mSelectedRhiNameIndex = ini_find_property(mIni, INI_GLOBAL_SECTION, "SelectedRhiName", 0);
			mSelectedExampleNameIndex = ini_find_property(mIni, INI_GLOBAL_SECTION, "SelectedExampleName", 0);
		}
	}
	if (nullptr == mIni)
	{
		mIni = ini_create(&renderer.getContext().getAllocator());
	}
}

void ImGuiExampleSelector::saveIni()
{
	if (nullptr != mIni)
	{
		const Renderer::IFileManager& fileManager = getRendererSafe().getFileManager();
		Renderer::IFile* file = fileManager.openFile(Renderer::IFileManager::FileMode::WRITE, ::ImGuiExampleSelectorDetail::VIRTUAL_SETTINGS_FILENAME);
		if (nullptr != file)
		{
			// Backup settings to ini
			if (INI_NOT_FOUND == mSelectedRhiNameIndex)
			{
				mSelectedRhiNameIndex = ini_property_add(mIni, INI_GLOBAL_SECTION, "SelectedRhiName", 0, mSelectedRhiName.c_str(), 0);
			}
			else
			{
				ini_property_value_set(mIni, INI_GLOBAL_SECTION, mSelectedRhiNameIndex, mSelectedRhiName.c_str(), 0);
			}
			if (INI_NOT_FOUND == mSelectedExampleNameIndex)
			{
				mSelectedExampleNameIndex = ini_property_add(mIni, INI_GLOBAL_SECTION, "SelectedExampleName", 0, mSelectedExampleName.c_str(), 0);
			}
			else
			{
				ini_property_value_set(mIni, INI_GLOBAL_SECTION, mSelectedExampleNameIndex, mSelectedExampleName.c_str(), 0);
			}

			// Save ini
			mIniFileContent.resize(static_cast<size_t>(ini_save(mIni, nullptr, 0)));
			ini_save(mIni, mIniFileContent.data(), static_cast<int>(mIniFileContent.size()));
			file->write(mIniFileContent.data(), mIniFileContent.size());
			fileManager.closeFile(*file);
		}
	}
}

void ImGuiExampleSelector::destroyIni()
{
	if (nullptr != mIni)
	{
		ini_destroy(mIni);
		mIni = nullptr;
		mSelectedRhiNameIndex = INI_NOT_FOUND;
		mSelectedExampleNameIndex = INI_NOT_FOUND;
	}
}

void ImGuiExampleSelector::createDebugGui()
{
	ImGui::SetNextWindowSize(ImVec2(260.0f, 100.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Example Selector"))
	{
		// Selection of RHI
		static int selectedRhiIndex = -1;
		int previouslySelectedRhiIndex = selectedRhiIndex;
		if (-1 == selectedRhiIndex && INI_NOT_FOUND != mSelectedRhiNameIndex)
		{
			// Restore the previously selected RHI from ini
			mSelectedRhiName = ini_property_value(mIni, INI_GLOBAL_SECTION, mSelectedRhiNameIndex);
		}
		{ // Fill list of RHI names
			std::string itemsSeparatedByZeros;
			const ExampleRunner::AvailableRhis& availableRhis = getExampleRunner().getAvailableRhis();
			{
				int rhiIndex = 0;
				const std::string& defaultRhiName = mSelectedRhiName.empty() ? getExampleRunner().getDefaultRhiName() : mSelectedRhiName;
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
						mSelectedRhiName = rhiName;
					}
					++rhiIndex;
				}
				itemsSeparatedByZeros += '\0';
			}

			// Tell ImGui
			ImGui::Combo("RHI", &selectedRhiIndex, itemsSeparatedByZeros.c_str());
			if (previouslySelectedRhiIndex != selectedRhiIndex)
			{
				// Update the selected RHI name at once
				ExampleRunner::AvailableRhis::const_iterator iterator = availableRhis.cbegin();
				std::advance(iterator, selectedRhiIndex);
				mSelectedRhiName = *iterator;
			}
		}

		{ // Selection of example
			static int selectedExampleIndex = -1;
			if (-1 == selectedExampleIndex && INI_NOT_FOUND != mSelectedExampleNameIndex)
			{
				// Restore the previously selected example from ini
				mSelectedExampleName = ini_property_value(mIni, INI_GLOBAL_SECTION, mSelectedExampleNameIndex);
			}
			else if (previouslySelectedRhiIndex != selectedRhiIndex)
			{
				// When changing the RHI the number of supported examples might change, try to keep the previously selected example selected
				selectedExampleIndex = -1;
			}

			// Fill list of examples supported by the currently selected RHI
			std::string itemsSeparatedByZeros;
			{
				int exampleIndex = 0;
				const std::string& defaultExampleName = mSelectedExampleName.empty() ? "Scene" : mSelectedExampleName;
				const ExampleRunner::ExampleToSupportedRhis& exampleToSupportedRhis = getExampleRunner().getExampleToSupportedRhis();
				for (const auto& pair : exampleToSupportedRhis)
				{
					const ExampleRunner::SupportedRhis& supportedRhiList = pair.second;
					if (pair.first != "ImGuiExampleSelector" && std::find(supportedRhiList.begin(), supportedRhiList.end(), mSelectedRhiName) != supportedRhiList.end())
					{
						itemsSeparatedByZeros += pair.first;
						itemsSeparatedByZeros += '\0';
						if (-1 == selectedExampleIndex && defaultExampleName == pair.first)
						{
							// Set initially selected example index, "Scene" is preferred since it's the most advantaged example
							selectedExampleIndex = exampleIndex;
						}
						if (exampleIndex == selectedExampleIndex)
						{
							mSelectedExampleName = pair.first;
						}
						++exampleIndex;
					}
				}
				itemsSeparatedByZeros += '\0';

				// In case the default example isn't supported by the selected RHI, initially select the first best supported example
				if (-1 == selectedExampleIndex && 0 != exampleIndex)
				{
					selectedExampleIndex = 0;
					mSelectedExampleName = itemsSeparatedByZeros;
				}
			}

			// Tell ImGui
			ImGui::Combo("Example", &selectedExampleIndex, itemsSeparatedByZeros.c_str());
		}

		// Start selected example
		if (ImGui::Button("Start"))
		{
			switchExample(mSelectedExampleName.c_str(), mSelectedRhiName.c_str());
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
