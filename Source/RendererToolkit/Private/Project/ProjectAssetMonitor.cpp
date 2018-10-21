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
#include "RendererToolkit/Private/Project/ProjectAssetMonitor.h"
#include "RendererToolkit/Private/Project/ProjectImpl.h"
#include "RendererToolkit/Private/Context.h"

#include <RendererRuntime/Public/IRendererRuntime.h>
#include <RendererRuntime/Public/Core/Platform/PlatformManager.h>

#include <FileWatcher/FileWatcher.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererToolkit
{


	namespace detail
	{
		class FileWatchListener final : public FW::FileWatchListener
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			explicit FileWatchListener(ProjectAssetMonitor& projectAssetMonitor) :
				mProjectAssetMonitor(projectAssetMonitor)
			{
				// Nothing here
			}

			FileWatchListener(const FileWatchListener&) = delete;

			virtual ~FileWatchListener()
			{
				// Nothing here
			}

			FileWatchListener& operator=(const FileWatchListener&) = delete;

			void processFileActions()
			{
				if (!mFileActions.empty())
				{
					// TODO(co) The current simple solution is not sufficient for large scale projects having ten thousands of assets: Add more efficient asset search which can handle asset dependencies as well
					/*
					const size_t numberOfFileActions = mFileActions.size();
					for (size_t fileActionIndex = 0; fileActionIndex < numberOfFileActions; ++fileActionIndex)
					{
						const FileAction& fileAction = mFileActions[fileActionIndex];
						*/
					{
						// Get the corresponding asset
						const RendererRuntime::AssetPackage::SortedAssetVector& sortedAssetVector = mProjectAssetMonitor.mProjectImpl.getAssetPackage().getSortedAssetVector();
						const size_t numberOfAssets = sortedAssetVector.size();
						for (size_t i = 0; i < numberOfAssets; ++i)
						{
							const RendererRuntime::Asset& asset = sortedAssetVector[i];
							try
							{
								if (mProjectAssetMonitor.mProjectImpl.checkAssetIsChanged(asset, mProjectAssetMonitor.mRendererTarget.c_str()))
								{
									// TODO(co) Performance: Add asset compiler queue so we can compile more then one asset at a time in background
									// TODO(co) At the moment, we only support modifying already existing asset data, we should add support for changes inside the runtime asset package as well
									RendererRuntime::AssetPackage outputAssetPackage;
									mProjectAssetMonitor.mProjectImpl.compileAssetIncludingDependencies(asset, mProjectAssetMonitor.mRendererTarget.c_str(), outputAssetPackage);

									// Inform the asset manager about the modified assets (just pass them individually, there's no real benefit in trying to apply "were's one, there are many" in this situation)
									const RendererRuntime::AssetPackage::SortedAssetVector& sortedOutputAssetVector = outputAssetPackage.getSortedAssetVector();
									const size_t numberOfOutputAssets = sortedOutputAssetVector.size();
									for (size_t outputAssetIndex = 0; outputAssetIndex < numberOfOutputAssets; ++outputAssetIndex)
									{
										mProjectAssetMonitor.mRendererRuntime.reloadResourceByAssetId(sortedOutputAssetVector[outputAssetIndex].assetId);
									}

									// A compilation run has been finished do cleanup
									mProjectAssetMonitor.mProjectImpl.onCompilationRunFinished();
									break;
								}
							}
							catch (const std::exception& e)
							{
								RENDERER_LOG(mProjectAssetMonitor.mProjectImpl.getContext(), CRITICAL, e.what())
							}

							// A compilation run has been finished do cleanup
							mProjectAssetMonitor.mProjectImpl.onCompilationRunFinished();
						}
					}
					mFileActions.clear();
				}
			}


		//[-------------------------------------------------------]
		//[ Public virtual FW::FileWatchListener methods          ]
		//[-------------------------------------------------------]
		public:
			void handleFileAction(FW::WatchID, const FW::String&, const FW::String& filename, FW::Action action) override
			{
				if (FW::Action::Modified == action)
				{
					// Sadly, we can and will get multiple modified events for one and the same modification, so we need to handle it in here
					const size_t numberOfFileActions = mFileActions.size();
					for (size_t fileActionIndex = 0; fileActionIndex < numberOfFileActions; ++fileActionIndex)
					{
						const FileAction& fileAction = mFileActions[fileActionIndex];
						if (fileAction.action == action && fileAction.filename == filename)
						{
							// No duplicates, please
							return;
						}
					}
					mFileActions.push_back(FileAction(filename, action));
				}
			}


		//[-------------------------------------------------------]
		//[ Private definitions                                   ]
		//[-------------------------------------------------------]
		private:
			struct FileAction final
			{
				FW::String filename;
				FW::Action action;
				FileAction(const FW::String& _filename, FW::Action _action) :
					filename(_filename),
					action(_action)
				{
					// Nothing here
				}
			};
			typedef std::vector<FileAction> FileActions;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			ProjectAssetMonitor& mProjectAssetMonitor;
			FileActions			 mFileActions;


		};
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	ProjectAssetMonitor::ProjectAssetMonitor(ProjectImpl& projectImpl, RendererRuntime::IRendererRuntime& rendererRuntime, const std::string& rendererTarget) :
		mProjectImpl(projectImpl),
		mRendererRuntime(rendererRuntime),
		mRendererTarget(rendererTarget),
		mShutdownThread(false)
	{
		mThread = std::thread(&ProjectAssetMonitor::threadWorker, this);
	}

	ProjectAssetMonitor::~ProjectAssetMonitor()
	{
		mShutdownThread = true;
		mThread.join();
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void ProjectAssetMonitor::threadWorker()
	{
		RENDERER_RUNTIME_SET_CURRENT_THREAD_DEBUG_NAME("Asset monitor", "Renderer toolkit: Project asset monitor");

		// Create the file watcher object
		FW::FileWatcher fileWatcher;
		detail::FileWatchListener fileWatchListener(*this);
		const FW::WatchID watchID = fileWatcher.addWatch(mProjectImpl.getAbsoluteProjectDirectory(), &fileWatchListener, true);

		// On startup we need to check for changes which were done while the project asset monitor wasn't running
		mProjectImpl.compileAllAssets(mRendererTarget.c_str());

		// Update the file watcher object as long as the project asset monitor is up-and-running
		while (!mShutdownThread)
		{
			fileWatcher.update();
			fileWatchListener.processFileActions();

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererToolkit
