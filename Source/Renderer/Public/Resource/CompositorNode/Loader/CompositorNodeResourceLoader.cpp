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
#include "Renderer/Public/Resource/CompositorNode/Loader/CompositorNodeResourceLoader.h"
#include "Renderer/Public/Resource/CompositorNode/Loader/CompositorNodeFileFormat.h"
#include "Renderer/Public/Resource/CompositorNode/Pass/ICompositorResourcePass.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeResourceManager.h"
#include "Renderer/Public/Resource/CompositorNode/CompositorNodeResource.h"


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
		void nodeTargetDeserialization(Renderer::IFile& file, Renderer::CompositorNodeResource& compositorNodeResource, const Renderer::ICompositorPassFactory& compositorPassFactory)
		{
			// Read in the compositor node resource target
			Renderer::v1CompositorNode::Target target;
			file.read(&target, sizeof(Renderer::v1CompositorNode::Target));

			// Create the compositor node resource target instance
			Renderer::CompositorTarget& compositorTarget = compositorNodeResource.addCompositorTarget(target.compositorChannelId, target.compositorFramebufferId);

			// Read in the compositor resource node target passes
			compositorTarget.setNumberOfCompositorResourcePasses(target.numberOfPasses);
			for (uint32_t i = 0; i < target.numberOfPasses; ++i)
			{
				// Read the pass header
				Renderer::v1CompositorNode::PassHeader passHeader;
				file.read(&passHeader, sizeof(Renderer::v1CompositorNode::PassHeader));

				// Create the compositor resource pass
				Renderer::ICompositorResourcePass* compositorResourcePass = compositorTarget.addCompositorResourcePass(compositorPassFactory, passHeader.compositorPassTypeId);

				// Deserialize the compositor resource pass
				if (nullptr != compositorResourcePass && 0 != passHeader.numberOfBytes)
				{
					// Load in the compositor resource pass data
					// TODO(co) Get rid of the new/delete in here
					uint8_t* data = new uint8_t[passHeader.numberOfBytes];
					file.read(data, passHeader.numberOfBytes);

					// Deserialize the compositor resource pass
					compositorResourcePass->deserialize(passHeader.numberOfBytes, data);

					// Cleanup
					delete [] data;
				}
				else
				{
					// TODO(co) Error handling
				}
			}
		}

		void nodeDeserialization(Renderer::IFile& file, const Renderer::v1CompositorNode::CompositorNodeHeader& compositorNodeHeader, Renderer::CompositorNodeResource& compositorNodeResource, const Renderer::ICompositorPassFactory& compositorPassFactory)
		{
			// Read in the compositor resource node input channels
			// TODO(co) Read all input channels in a single burst? (need to introduce a maximum number of input channels for this)
			compositorNodeResource.reserveInputChannels(compositorNodeHeader.numberOfInputChannels);
			for (uint32_t i = 0; i < compositorNodeHeader.numberOfInputChannels; ++i)
			{
				Renderer::CompositorChannelId channelId;
				file.read(&channelId, sizeof(Renderer::CompositorChannelId));
				compositorNodeResource.addInputChannel(channelId);
			}

			// TODO(co) Read all render target textures in a single burst?
			compositorNodeResource.reserveRenderTargetTextures(compositorNodeHeader.numberOfRenderTargetTextures);
			for (uint32_t i = 0; i < compositorNodeHeader.numberOfRenderTargetTextures; ++i)
			{
				Renderer::v1CompositorNode::RenderTargetTexture renderTargetTexture;
				file.read(&renderTargetTexture, sizeof(Renderer::v1CompositorNode::RenderTargetTexture));
				compositorNodeResource.addRenderTargetTexture(renderTargetTexture.assetId, renderTargetTexture.renderTargetTextureSignature);
			}

			// TODO(co) Read all framebuffers in a single burst?
			compositorNodeResource.reserveFramebuffers(compositorNodeHeader.numberOfFramebuffers);
			for (uint32_t i = 0; i < compositorNodeHeader.numberOfFramebuffers; ++i)
			{
				Renderer::v1CompositorNode::Framebuffer framebuffer;
				file.read(&framebuffer, sizeof(Renderer::v1CompositorNode::Framebuffer));
				compositorNodeResource.addFramebuffer(framebuffer.compositorFramebufferId, framebuffer.framebufferSignature);
			}

			// Read in the compositor node resource targets
			compositorNodeResource.reserveCompositorTargets(compositorNodeHeader.numberOfTargets);
			for (uint32_t i = 0; i < compositorNodeHeader.numberOfTargets; ++i)
			{
				nodeTargetDeserialization(file, compositorNodeResource, compositorPassFactory);
			}

			// Read in the compositor resource node output channels
			// TODO(co) Read all output channels in a single burst? (need to introduce a maximum number of output channels for this)
			compositorNodeResource.reserveOutputChannels(compositorNodeHeader.numberOfOutputChannels);
			for (uint32_t i = 0; i < compositorNodeHeader.numberOfOutputChannels; ++i)
			{
				Renderer::CompositorChannelId channelId;
				file.read(&channelId, sizeof(Renderer::CompositorChannelId));
				compositorNodeResource.addOutputChannel(channelId);
			}
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
	void CompositorNodeResourceLoader::initialize(const Asset& asset, bool reload, IResource& resource)
	{
		IResourceLoader::initialize(asset, reload);
		mCompositorNodeResource = static_cast<CompositorNodeResource*>(&resource);
	}

	bool CompositorNodeResourceLoader::onDeserialization(IFile& file)
	{
		// Tell the memory mapped file about the LZ4 compressed data
		return mMemoryFile.loadLz4CompressedDataFromFile(v1CompositorNode::FORMAT_TYPE, v1CompositorNode::FORMAT_VERSION, file);
	}

	void CompositorNodeResourceLoader::onProcessing()
	{
		// Decompress LZ4 compressed data
		mMemoryFile.decompress();

		// Read in the compositor node header
		v1CompositorNode::CompositorNodeHeader compositorNodeHeader;
		mMemoryFile.read(&compositorNodeHeader, sizeof(v1CompositorNode::CompositorNodeHeader));

		// Read in the compositor node resource
		::detail::nodeDeserialization(mMemoryFile, compositorNodeHeader, *mCompositorNodeResource, static_cast<CompositorNodeResourceManager&>(getResourceManager()).getCompositorPassFactory());
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
