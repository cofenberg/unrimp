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
#include <RendererRuntime/Public/Core/File/IFile.h>
#include <RendererRuntime/Public/Core/File/IFileManager.h>
#include <RendererRuntime/Public/Core/Platform/PlatformManager.h>

#include <Renderer/Public/DefaultLog.h>

#include <imgui/imgui.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class IFileManager;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    ImGui log implementation class one can use
	*
	*  @note
	*    - Designed to be instanced and used inside a single C++ file
	*    - Basing on "Tip/Demo: Log example as helper class. #300" - https://github.com/ocornut/imgui/issues/300
	*/
	class ImGuiLog final : public Renderer::DefaultLog
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		inline ImGuiLog() :
			mScrollToBottom(false),
			mOpen(false)
		{
			// Nothing here
		}

		inline virtual ~ImGuiLog() override
		{
			// Nothing here
		}

		inline void open()
		{
			mOpen = true;
			ImGui::SetWindowCollapsed("Log", false);
		}

		inline void clear()
		{
			// Since it should be possible to use this class via static global instances, ensure that clear frees allocated memory to not cause false-positive memory leaks being detected
			mImGuiTextBuffer.Buf.clear();	// "mImGuiTextBuffer.clear();" will add a zero after clearing, so don't use it
			mEntries.clear();
			mEntries.shrink_to_fit();	// At least when using Visual Studio, this will also free allocated memory
		}

		inline void draw(IFileManager& fileManager)
		{
			if (mOpen)
			{
				ImGui::SetNextWindowSize(ImVec2(500.0f, 400.0f), ImGuiCond_FirstUseEver);
				ImGui::Begin("Log", &mOpen);
				if (ImGui::Button("Clear"))
				{
					clear();
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Clear log");
				}
				ImGui::SameLine();
				const bool copy = ImGui::Button("Copy");
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Copy log to operation system clipboard");
				}
				ImGui::SameLine();
				mImGuiTextFilter.Draw("Filter", -100.0f);
				ImGui::Separator();
				ImGui::BeginChild("scrolling");
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 1.0f));
				ImGui::PushTextWrapPos(0.0f);
				ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
				if (copy)
				{
					ImGui::LogToClipboard();
				}
				if (!mImGuiTextBuffer.empty())
				{
					// TODO(co) Optimization: With huge logs the current trivial implementation will certainly become a bottleneck
					const char* bufferBegin = mImGuiTextBuffer.begin();
					const char* line = bufferBegin;
					for (size_t lineNumber = 0; nullptr != line; ++lineNumber)
					{
						const char* lineEnd = (lineNumber < mEntries.size()) ? (bufferBegin + mEntries[lineNumber].lineOffsets) : nullptr;
						if (!mImGuiTextFilter.IsActive() || mImGuiTextFilter.PassFilter(line, lineEnd))
						{
							ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);
							const Entry& entry = mEntries[lineNumber];
							switch (entry.type)
							{
								case Renderer::ILog::Type::TRACE:
								case Renderer::ILog::Type::DEBUG:
									color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
									break;

								case Renderer::ILog::Type::INFORMATION:
									color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
									break;

								case Renderer::ILog::Type::WARNING:
								case Renderer::ILog::Type::PERFORMANCE_WARNING:
								case Renderer::ILog::Type::COMPATIBILITY_WARNING:
									color = ImVec4(0.5f, 0.5f, 1.0f, 1.0f);
									break;

								case Renderer::ILog::Type::CRITICAL:
									color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
									if (!entry.attachment.empty())
									{
										if (ImGui::Button(("!" + std::to_string(lineNumber)).c_str()))
										{
											// Copy log entry attachment to operation system clipboard
											ImGui::SetClipboardText(entry.attachment.c_str());

											// Try to open the content as file in e.g. inside a text editor to make it possible to review e.g. shader issues as fast as possible
											if (nullptr != fileManager.getLocalDataMountPoint() && fileManager.createDirectories(fileManager.getLocalDataMountPoint()))
											{
												const std::string virtualFilename = std::string(fileManager.getLocalDataMountPoint()) + "/TemporaryLogAttachment.txt";
												IFile* file = fileManager.openFile(IFileManager::FileMode::WRITE, virtualFilename.c_str());
												if (nullptr != file)
												{
													file->write(entry.attachment.data(), entry.attachment.size());

													// Close file
													fileManager.closeFile(*file);

													// Try to open the content as file in e.g. inside a text editor
													PlatformManager::openUrlExternal(("file://" + fileManager.mapVirtualToAbsoluteFilename(IFileManager::FileMode::READ, virtualFilename.c_str())).c_str());
												}
												else
												{
													// Error!
													if (print(Renderer::ILog::Type::CRITICAL, nullptr, __FILE__, static_cast<uint32_t>(__LINE__), "Failed to open the file \"%s\" for writing", virtualFilename.c_str()))
													{
														DEBUG_BREAK;
													}
												}
											}
										}
										if (ImGui::IsItemHovered())
										{
											ImGui::SetTooltip("Copy log entry attachment to operation system clipboard and try to open the content as file in e.g. inside a text editor");
										}
										ImGui::SameLine();
									}
									break;
							}
							ImGui::PushStyleColor(ImGuiCol_Text, color);
							ImGui::TextUnformatted(line, lineEnd);
							ImGui::PopStyleColor();
						}
						line = (lineEnd && lineEnd[1]) ? (lineEnd + 1) : nullptr;
					}
				}
				if (mScrollToBottom)
				{
					ImGui::SetScrollHereY(1.0f);
				}
				mScrollToBottom = false;
				ImGui::PopTextWrapPos();
				ImGui::PopStyleVar();
				ImGui::EndChild();
				ImGui::End();
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::DefaultLog methods        ]
	//[-------------------------------------------------------]
	protected:
		[[nodiscard]] inline virtual bool printInternal(Type type, const char* attachment, const char* file, uint32_t line, const char* message, uint32_t numberOfCharacters) override
		{
			// Call the base implementation
			const bool requestDebugBreak = DefaultLog::printInternal(type, attachment, file, line, message, numberOfCharacters);

			// Construct the full UTF-8 message text
			#ifdef _DEBUG
				std::string fullMessage = "File \"" + std::string(file) + "\" | Line " + std::to_string(line) + " | " + std::string(typeToString(type)) + message;
			#else
				std::string fullMessage = std::string(typeToString(type)) + message;
			#endif
			if ('\n' != fullMessage.back())
			{
				fullMessage += '\n';
			}

			// Add to ImGui log
			int previousSize = mImGuiTextBuffer.size();
			mImGuiTextBuffer.appendf("%s", fullMessage.c_str());
			for (int newSize = mImGuiTextBuffer.size(); previousSize < newSize; ++previousSize)
			{
				if ('\n' == mImGuiTextBuffer[previousSize])
				{
					Entry entry;
					entry.lineOffsets = previousSize;
					entry.type = type;
					if (nullptr != attachment)
					{
						entry.attachment = attachment;

						// Show attachment only once per log entry
						attachment = nullptr;
					}
					mEntries.push_back(entry);
				}
			}
			mScrollToBottom = true;

			// Open the log automatically on warning or error
			switch (type)
			{
				case Renderer::ILog::Type::TRACE:
				case Renderer::ILog::Type::DEBUG:
				case Renderer::ILog::Type::INFORMATION:
					// Nothing here
					break;

				case Renderer::ILog::Type::WARNING:
				case Renderer::ILog::Type::PERFORMANCE_WARNING:
				case Renderer::ILog::Type::COMPATIBILITY_WARNING:
				case Renderer::ILog::Type::CRITICAL:
					// ImGui might not have been initialized yet
					if (nullptr != ImGui::GetCurrentContext())
					{
						open();
					}
					break;
			}

			// Done
			return requestDebugBreak;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ImGuiLog(const ImGuiLog&) = delete;
		ImGuiLog& operator=(const ImGuiLog&) = delete;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct Entry final
		{
			int			lineOffsets;	///< Index to lines offset
			Type		type;
			std::string	attachment;		///< Optional attachment (for example build shader source code)
		};


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		ImGuiTextBuffer	   mImGuiTextBuffer;
		ImGuiTextFilter	   mImGuiTextFilter;
		std::vector<Entry> mEntries;
		bool			   mScrollToBottom;
		bool			   mOpen;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
