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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Resource/CompositorWorkspace/Loader/CompositorWorkspaceResourceLoader.h"
#include "Renderer/Public/Resource/CompositorWorkspace/Loader/CompositorWorkspaceFileFormat.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceResource.h"


// TODO(co) Error handling


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void nodesDeserialization(Renderer::IFile& file, Renderer::CompositorWorkspaceResource& compositorWorkspaceResource)
		{
			// Read in the compositor workspace resource nodes
			Renderer::v1CompositorWorkspace::Nodes nodes;
			file.read(&nodes, sizeof(Renderer::v1CompositorWorkspace::Nodes));

			// Sanity check
			ASSERT((nodes.numberOfNodes > 0) && "Invalid compositor workspace asset without any nodes detected");

			// Read in the compositor node asset IDs
			compositorWorkspaceResource.reserveCompositorNodes(nodes.numberOfNodes);
			// TODO(co) Get rid of the evil const-cast
			Renderer::CompositorWorkspaceResource::CompositorNodeAssetIds& compositorNodeAssetIds = const_cast<Renderer::CompositorWorkspaceResource::CompositorNodeAssetIds&>(compositorWorkspaceResource.getCompositorNodeAssetIds());
			compositorNodeAssetIds.resize(nodes.numberOfNodes);
			file.read(compositorNodeAssetIds.data(), sizeof(Renderer::AssetId) * nodes.numberOfNodes);
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceLoader methods      ]
	//[-------------------------------------------------------]
	void CompositorWorkspaceResourceLoader::initialize(const Asset& asset, bool reload, IResource& resource)
	{
		IResourceLoader::initialize(asset, reload);
		mCompositorWorkspaceResource = static_cast<CompositorWorkspaceResource*>(&resource);
	}

	bool CompositorWorkspaceResourceLoader::onDeserialization(IFile& file)
	{
		// Tell the memory mapped file about the LZ4 compressed data
		return mMemoryFile.loadLz4CompressedDataFromFile(v1CompositorWorkspace::FORMAT_TYPE, v1CompositorWorkspace::FORMAT_VERSION, file);
	}

	void CompositorWorkspaceResourceLoader::onProcessing()
	{
		// Decompress LZ4 compressed data
		mMemoryFile.decompress();

		// Read in the compositor workspace header
		// TODO(co) Currently the compositor workspace header is unused
		v1CompositorWorkspace::CompositorWorkspaceHeader compositorWorkspaceHeader;
		mMemoryFile.read(&compositorWorkspaceHeader, sizeof(v1CompositorWorkspace::CompositorWorkspaceHeader));

		// Read in the compositor workspace resource nodes
		::detail::nodesDeserialization(mMemoryFile, *mCompositorWorkspaceResource);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
