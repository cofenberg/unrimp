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
#include "RendererRuntime/Public/DebugGui/DebugGuiManager.h"
#include "RendererRuntime/Public/DebugGui/DebugGuiHelper.h"
#include "RendererRuntime/Public/Resource/Texture/TextureResourceManager.h"
#include "RendererRuntime/Public/Core/File/IFileManager.h"
#include "RendererRuntime/Public/Core/IProfiler.h"
#include "RendererRuntime/Public/IRendererRuntime.h"
#include "RendererRuntime/Public/Context.h"

#include <ImGuizmo/ImGuizmo.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/detail/setup.hpp>	// For "GLM_COUNTOF()"
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		// Vertex input layout
		static constexpr Rhi::VertexAttribute VertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Rhi::VertexAttributeFormat::FLOAT_2,		// vertexAttributeFormat (Rhi::VertexAttributeFormat)
				"Position",									// name[32] (char)
				"POSITION",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				0,											// alignedByteOffset (uint32_t)
				sizeof(float) * 4 + sizeof(uint8_t) * 4,	// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			},
			{ // Attribute 1
				// Data destination
				Rhi::VertexAttributeFormat::FLOAT_2,		// vertexAttributeFormat (Rhi::VertexAttributeFormat)
				"TexCoord",									// name[32] (char)
				"TEXCOORD",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				sizeof(float) * 2,							// alignedByteOffset (uint32_t)
				sizeof(float) * 4 + sizeof(uint8_t) * 4,	// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			},
			{ // Attribute 2
				// Data destination
				Rhi::VertexAttributeFormat::R8G8B8A8_UNORM,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
				"Color",									// name[32] (char)
				"COLOR",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				sizeof(float) * 4,							// alignedByteOffset (uint32_t)
				sizeof(float) * 4 + sizeof(uint8_t) * 4,	// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			}
		};
		const Rhi::VertexAttributes VertexAttributes(static_cast<uint32_t>(GLM_COUNTOF(VertexAttributesLayout)), VertexAttributesLayout);


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		[[nodiscard]] void* AllocFunc(size_t sz, void* user_data)
		{
			return static_cast<Rhi::IAllocator*>(user_data)->reallocate(nullptr, 0, sz, 1);
		}

		void FreeFunc(void* ptr, void* user_data)
		{
			static_cast<Rhi::IAllocator*>(user_data)->reallocate(ptr, 0, 0, 1);
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	void DebugGuiManager::setImGuiAllocatorFunctions(Rhi::IAllocator& allocator)
	{
		ImGui::SetAllocatorFunctions(::detail::AllocFunc, ::detail::FreeFunc, &allocator);
	}

	void DebugGuiManager::getDefaultTextureAssetIds(AssetIds& assetIds)
	{
		// Define helper macro
		#define ADD_ASSET_ID(name) assetIds.push_back(ASSET_ID(name));

		// Add asset IDs
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/ImGuiGlyphMap2D")

		// Undefine helper macro
		#undef ADD_ASSET_ID
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void DebugGuiManager::newFrame(Rhi::IRenderTarget& renderTarget, CompositorWorkspaceInstance* compositorWorkspaceInstance)
	{
		// Startup the debug GUI manager now?
		if (!mIsRunning)
		{
			startup();
			mIsRunning = true;
		}

		// Call the platform specific implementation
		onNewFrame(renderTarget);

		// Start the frame
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
		DebugGuiHelper::beginFrame();
		if (mOpenMetricsWindow)
		{
			DebugGuiHelper::drawMetricsWindow(mOpenMetricsWindow, compositorWorkspaceInstance);
		}
	}

	const Rhi::IVertexArrayPtr& DebugGuiManager::getFillVertexArrayPtr()
	{
		if (GImGui->Initialized)
		{
			// Ask ImGui to render into the internal command buffer and then request the resulting draw data
			ImGui::Render();
			const ImDrawData* imDrawData = ImGui::GetDrawData();
			Rhi::IRhi& rhi = mRendererRuntime.getRhi();
			Rhi::IBufferManager& bufferManager = mRendererRuntime.getBufferManager();

			{ // Vertex and index buffers
				// Create and grow vertex/index buffers if needed
				if (nullptr == mVertexBufferPtr || mNumberOfAllocatedVertices < static_cast<uint32_t>(imDrawData->TotalVtxCount))
				{
					mNumberOfAllocatedVertices = static_cast<uint32_t>(imDrawData->TotalVtxCount + 5000);	// Add some reserve to reduce reallocations
					mVertexBufferPtr = bufferManager.createVertexBuffer(mNumberOfAllocatedVertices * sizeof(ImDrawVert), nullptr, 0, Rhi::BufferUsage::DYNAMIC_DRAW);
					RHI_SET_RESOURCE_DEBUG_NAME(mVertexBufferPtr, "Debug GUI")
					mVertexArrayPtr = nullptr;
				}
				if (nullptr == mIndexBufferPtr || mNumberOfAllocatedIndices < static_cast<uint32_t>(imDrawData->TotalIdxCount))
				{
					mNumberOfAllocatedIndices = static_cast<uint32_t>(imDrawData->TotalIdxCount + 10000);	// Add some reserve to reduce reallocations
					mIndexBufferPtr = bufferManager.createIndexBuffer(mNumberOfAllocatedIndices * sizeof(ImDrawIdx), nullptr, 0, Rhi::BufferUsage::DYNAMIC_DRAW);
					RHI_SET_RESOURCE_DEBUG_NAME(mIndexBufferPtr, "Debug GUI")
					mVertexArrayPtr = nullptr;
				}
				if (nullptr == mVertexArrayPtr)
				{
					RHI_ASSERT(mRendererRuntime.getContext(), nullptr != mVertexBufferPtr, "Invalid vertex buffer")
					RHI_ASSERT(mRendererRuntime.getContext(), nullptr != mIndexBufferPtr, "Invalid index buffer")

					// Create vertex array object (VAO)
					const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { mVertexBufferPtr };
					mVertexArrayPtr = bufferManager.createVertexArray(::detail::VertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, mIndexBufferPtr);
					RHI_SET_RESOURCE_DEBUG_NAME(mVertexArrayPtr, "Debug GUI")
				}

				{ // Copy and convert all vertices and indices into a single contiguous buffer
					Rhi::MappedSubresource vertexBufferMappedSubresource;
					if (rhi.map(*mVertexBufferPtr, 0, Rhi::MapType::WRITE_DISCARD, 0, vertexBufferMappedSubresource))
					{
						Rhi::MappedSubresource indexBufferMappedSubresource;
						if (rhi.map(*mIndexBufferPtr, 0, Rhi::MapType::WRITE_DISCARD, 0, indexBufferMappedSubresource))
						{
							ImDrawVert* imDrawVert = static_cast<ImDrawVert*>(vertexBufferMappedSubresource.data);
							ImDrawIdx* imDrawIdx = static_cast<ImDrawIdx*>(indexBufferMappedSubresource.data);
							for (int i = 0; i < imDrawData->CmdListsCount; ++i)
							{
								const ImDrawList* imDrawList = imDrawData->CmdLists[i];
								memcpy(imDrawVert, &imDrawList->VtxBuffer[0], imDrawList->VtxBuffer.size() * sizeof(ImDrawVert));
								memcpy(imDrawIdx, &imDrawList->IdxBuffer[0], imDrawList->IdxBuffer.size() * sizeof(ImDrawIdx));
								imDrawVert += imDrawList->VtxBuffer.size();
								imDrawIdx += imDrawList->IdxBuffer.size();
							}

							// Unmap the index buffer
							rhi.unmap(*mIndexBufferPtr, 0);
						}

						// Unmap the vertex buffer
						rhi.unmap(*mVertexBufferPtr, 0);
					}
				}
			}
		}

		// Done
		return mVertexArrayPtr;
	}

	void DebugGuiManager::fillGraphicsCommandBuffer(Rhi::CommandBuffer& commandBuffer)
	{
		if (GImGui->Initialized)
		{
			// No combined scoped profiler CPU and GPU sample as well as renderer debug event command by intent, this is something the caller has to take care of
			// RENDERER_SCOPED_PROFILER_EVENT(mRendererRuntime.getContext(), commandBuffer, "Debug GUI")

			// Render command lists
			// -> There's no need to try to gather draw calls and batch them into multi-draw-indirect buffers, ImGui does already a pretty good job
			int vertexOffset = 0;
			int indexOffset = 0;
			const ImDrawData* imDrawData = ImGui::GetDrawData();
			for (int commandListIndex = 0; commandListIndex < imDrawData->CmdListsCount; ++commandListIndex)
			{
				const ImDrawList* imDrawList = imDrawData->CmdLists[commandListIndex];
				for (int commandIndex = 0; commandIndex < imDrawList->CmdBuffer.size(); ++commandIndex)
				{
					const ImDrawCmd* pcmd = &imDrawList->CmdBuffer[commandIndex];
					if (nullptr != pcmd->UserCallback)
					{
						pcmd->UserCallback(imDrawList, pcmd);
					}
					else
					{
						// Set graphics scissor rectangle
						Rhi::Command::SetGraphicsScissorRectangles::create(commandBuffer, static_cast<long>(pcmd->ClipRect.x), static_cast<long>(pcmd->ClipRect.y), static_cast<long>(pcmd->ClipRect.z), static_cast<long>(pcmd->ClipRect.w));

						// Draw graphics
						Rhi::Command::DrawIndexedGraphics::create(commandBuffer, static_cast<uint32_t>(pcmd->ElemCount), 1, static_cast<uint32_t>(indexOffset), static_cast<int32_t>(vertexOffset));
					}
					indexOffset += pcmd->ElemCount;
				}
				vertexOffset += imDrawList->VtxBuffer.size();
			}
		}
	}

	void DebugGuiManager::fillGraphicsCommandBufferUsingFixedBuildInRhiConfiguration(Rhi::CommandBuffer& commandBuffer)
	{
		if (GImGui->Initialized)
		{
			// No combined scoped profiler CPU and GPU sample as well as renderer debug event command by intent, this is something the caller has to take care of
			// RENDERER_SCOPED_PROFILER_EVENT(mRendererRuntime.getContext(), commandBuffer, "Fixed debug GUI")

			// Create fixed build in RHI configuration resources, if required
			if (nullptr == mRootSignature)
			{
				createFixedBuildInRhiConfigurationResources();
			}

			{ // Setup orthographic projection matrix into our vertex shader uniform buffer
				const ImGuiIO& imGuiIo = ImGui::GetIO();
				float objectSpaceToClipSpaceMatrix[4][4] =
				{
					{  2.0f / imGuiIo.DisplaySize.x, 0.0f,                          0.0f, 0.0f },
					{  0.0f,                         2.0f / -imGuiIo.DisplaySize.y, 0.0f, 0.0f },
					{  0.0f,                         0.0f,                          0.5f, 0.0f },
					{ -1.0f,                         1.0f,                          0.5f, 1.0f }
				};

				// Copy data
				// TODO(co) Since the data copy isn't performed via commands, we better manage it somehow to ensure no problems come up when the following is executed multiple times per frame (which usually isn't the case)
				if (nullptr != mVertexShaderUniformBuffer)
				{
					Rhi::MappedSubresource mappedSubresource;
					Rhi::IRhi& rhi = mRendererRuntime.getRhi();
					if (rhi.map(*mVertexShaderUniformBuffer, 0, Rhi::MapType::WRITE_DISCARD, 0, mappedSubresource))
					{
						memcpy(mappedSubresource.data, objectSpaceToClipSpaceMatrix, sizeof(objectSpaceToClipSpaceMatrix));
						rhi.unmap(*mVertexShaderUniformBuffer, 0);
					}
				}
				else
				{
					// TODO(co) Not compatible with command buffer: This certainly is going to be removed, we need to implement internal uniform buffer emulation
					mGraphicsProgram->setUniformMatrix4fv(mObjectSpaceToClipSpaceMatrixUniformHandle, &objectSpaceToClipSpaceMatrix[0][0]);
				}
			}

			{ // RHI configuration
				// Set the used graphics root signature
				Rhi::Command::SetGraphicsRootSignature::create(commandBuffer, mRootSignature);

				// Set the used graphics pipeline state object (PSO)
				Rhi::Command::SetGraphicsPipelineState::create(commandBuffer, mGraphicsPipelineState);

				// Set graphics resource groups
				Rhi::Command::SetGraphicsResourceGroup::create(commandBuffer, 0, mResourceGroup);
				Rhi::Command::SetGraphicsResourceGroup::create(commandBuffer, 1, mSamplerStateGroup);
			}

			// Setup input assembly (IA): Set the used vertex array
			Rhi::Command::SetGraphicsVertexArray::create(commandBuffer, getFillVertexArrayPtr());

			// Render command lists
			fillGraphicsCommandBuffer(commandBuffer);
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::DebugGuiManager methods ]
	//[-------------------------------------------------------]
	void DebugGuiManager::startup()
	{
		RHI_ASSERT(mRendererRuntime.getContext(), !mIsRunning, "The debug GUI manager is already running")

		{ // Create texture instance
			// Build texture atlas
			unsigned char* pixels = nullptr;
			int width = 0;
			int height = 0;
			ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

			// Upload texture to RHI
			mTexture2D = mRendererRuntime.getTextureManager().createTexture2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), Rhi::TextureFormat::R8, pixels, Rhi::TextureFlag::GENERATE_MIPMAPS | Rhi::TextureFlag::SHADER_RESOURCE);
			RHI_SET_RESOURCE_DEBUG_NAME(mTexture2D, "Debug 2D GUI glyph texture atlas")

			// Tell the texture resource manager about our render target texture so it can be referenced inside e.g. compositor nodes
			mRendererRuntime.getTextureResourceManager().createTextureResourceByAssetId(ASSET_ID("Unrimp/Texture/DynamicByCode/ImGuiGlyphMap2D"), *mTexture2D);
		}
	}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	DebugGuiManager::DebugGuiManager(IRendererRuntime& rendererRuntime) :
		mRendererRuntime(rendererRuntime),
		mImGuiContext(nullptr),
		mIsRunning(false),
		mObjectSpaceToClipSpaceMatrixUniformHandle(NULL_HANDLE),
		mNumberOfAllocatedVertices(0),
		mNumberOfAllocatedIndices(0),
		mOpenMetricsWindow(false)
	{
		// Set ImGui allocator functions
		setImGuiAllocatorFunctions(rendererRuntime.getContext().getAllocator());

		// Create ImGui context
		mImGuiContext = ImGui::CreateContext();

		// Change ImGui filenames so one is able to guess where those files come from when using Unrimp
		const IFileManager& fileManager = rendererRuntime.getFileManager();
		const char* localDataMountPoint = fileManager.getLocalDataMountPoint();
		ImGuiIO& imGuiIo = ImGui::GetIO();
		imGuiIo.IniFilename = nullptr;
		imGuiIo.LogFilename = nullptr;
		if (nullptr != localDataMountPoint && fileManager.createDirectories(localDataMountPoint))
		{
			// ImGui has no file system abstraction and needs absolute filenames
			const std::string virtualDebugGuiDirectoryName = localDataMountPoint;
			mAbsoluteIniFilename = fileManager.mapVirtualToAbsoluteFilename(IFileManager::FileMode::WRITE, (virtualDebugGuiDirectoryName + "/UnrimpImGuiLayout.ini").c_str());
			mAbsoluteLogFilename = fileManager.mapVirtualToAbsoluteFilename(IFileManager::FileMode::WRITE, (virtualDebugGuiDirectoryName + "/UnrimpImGuiLog.txt").c_str());
			imGuiIo.IniFilename = mAbsoluteIniFilename.c_str();
			imGuiIo.LogFilename = mAbsoluteLogFilename.c_str();
		}

		// Setup ImGui style
		ImGui::StyleColorsDark();
	}

	DebugGuiManager::~DebugGuiManager()
	{
		ImGui::DestroyContext(mImGuiContext);
	}

	void DebugGuiManager::createFixedBuildInRhiConfigurationResources()
	{
		Rhi::IRhi& rhi = mRendererRuntime.getRhi();
		RHI_ASSERT(mRendererRuntime.getContext(), nullptr == mRootSignature, "The debug GUI manager has already root signature")

		{ // Create the root signature instance
			// Create the root signature
			Rhi::DescriptorRangeBuilder ranges[3];
			ranges[0].initialize(Rhi::ResourceType::UNIFORM_BUFFER, 0, "UniformBlockDynamicVs", Rhi::ShaderVisibility::VERTEX);
			ranges[1].initialize(Rhi::ResourceType::TEXTURE_2D,		0, "GlyphMap",				Rhi::ShaderVisibility::FRAGMENT);
			ranges[2].initializeSampler(0, Rhi::ShaderVisibility::FRAGMENT);

			Rhi::RootParameterBuilder rootParameters[2];
			rootParameters[0].initializeAsDescriptorTable(2, &ranges[0]);
			rootParameters[1].initializeAsDescriptorTable(1, &ranges[2]);

			// Setup
			Rhi::RootSignatureBuilder rootSignature;
			rootSignature.initialize(static_cast<uint32_t>(GLM_COUNTOF(rootParameters)), rootParameters, 0, nullptr, Rhi::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mRootSignature = rhi.createRootSignature(rootSignature);
			RHI_SET_RESOURCE_DEBUG_NAME(mRootSignature, "Debug GUI")
		}

		{ // Create the graphics pipeline state instance
			{ // Create the graphics program
				// Get the shader source code (outsourced to keep an overview)
				const char* vertexShaderSourceCode = nullptr;
				const char* fragmentShaderSourceCode = nullptr;
				#include "Detail/Shader/DebugGui_GLSL_450.h"	// For Vulkan
				#include "Detail/Shader/DebugGui_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
				#include "Detail/Shader/DebugGui_GLSL_ES3.h"
				#include "Detail/Shader/DebugGui_HLSL_D3D9.h"
				#include "Detail/Shader/DebugGui_HLSL_D3D10_D3D11_D3D12.h"
				#include "Detail/Shader/DebugGui_Null.h"

				// Create the shaders
				Rhi::IShaderLanguage& shaderLanguage = rhi.getDefaultShaderLanguage();
				Rhi::IVertexShader* vertexShader = shaderLanguage.createVertexShaderFromSourceCode(::detail::VertexAttributes, vertexShaderSourceCode);
				RHI_SET_RESOURCE_DEBUG_NAME(vertexShader, "Debug GUI")
				Rhi::IFragmentShader* fragmentShader = shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode);
				RHI_SET_RESOURCE_DEBUG_NAME(fragmentShader, "Debug GUI")

				// Create the graphics program
				mGraphicsProgram = shaderLanguage.createGraphicsProgram(
					*mRootSignature,
					::detail::VertexAttributes,
					vertexShader,
					fragmentShader);
				RHI_SET_RESOURCE_DEBUG_NAME(mGraphicsProgram, "Debug GUI")
			}

			// Create the graphics pipeline state object (PSO)
			if (nullptr != mGraphicsProgram)
			{
				// TODO(co) Render pass related update, the render pass in here is currently just a dummy so the debug compositor works
				Rhi::IRenderPass* renderPass = rhi.createRenderPass(1, &rhi.getCapabilities().preferredSwapChainColorTextureFormat, rhi.getCapabilities().preferredSwapChainDepthStencilTextureFormat);

				Rhi::GraphicsPipelineState graphicsPipelineState = Rhi::GraphicsPipelineStateBuilder(mRootSignature, mGraphicsProgram, ::detail::VertexAttributes, *renderPass);
				graphicsPipelineState.rasterizerState.cullMode				   = Rhi::CullMode::NONE;
				graphicsPipelineState.rasterizerState.scissorEnable			   = 1;
				graphicsPipelineState.depthStencilState.depthEnable			   = false;
				graphicsPipelineState.depthStencilState.depthWriteMask		   = Rhi::DepthWriteMask::ZERO;
				graphicsPipelineState.blendState.renderTarget[0].blendEnable   = true;
				graphicsPipelineState.blendState.renderTarget[0].srcBlend	   = Rhi::Blend::SRC_ALPHA;
				graphicsPipelineState.blendState.renderTarget[0].destBlend	   = Rhi::Blend::INV_SRC_ALPHA;
				graphicsPipelineState.blendState.renderTarget[0].srcBlendAlpha = Rhi::Blend::INV_SRC_ALPHA;
				mGraphicsPipelineState = rhi.createGraphicsPipelineState(graphicsPipelineState);
				RHI_SET_RESOURCE_DEBUG_NAME(mGraphicsPipelineState, "Debug GUI")
			}
		}

		// Create vertex uniform buffer instance
		if (rhi.getCapabilities().maximumUniformBufferSize > 0)
		{
			mVertexShaderUniformBuffer = mRendererRuntime.getBufferManager().createUniformBuffer(sizeof(float) * 4 * 4, nullptr, Rhi::BufferUsage::DYNAMIC_DRAW);
			RHI_SET_RESOURCE_DEBUG_NAME(mVertexShaderUniformBuffer, "Debug GUI")
		}
		else if (nullptr != mGraphicsProgram)
		{
			mObjectSpaceToClipSpaceMatrixUniformHandle = mGraphicsProgram->getUniformHandle("ObjectSpaceToClipSpaceMatrix");
		}

		// Create sampler state instance and wrap it into a resource group instance
		Rhi::IResource* samplerStateResource = nullptr;
		{
			Rhi::SamplerState samplerState = Rhi::ISamplerState::getDefaultSamplerState();
			samplerState.addressU = Rhi::TextureAddressMode::WRAP;
			samplerState.addressV = Rhi::TextureAddressMode::WRAP;
			samplerStateResource = rhi.createSamplerState(samplerState);
			RHI_SET_RESOURCE_DEBUG_NAME(samplerStateResource, "Debug GUI")
			mSamplerStateGroup = mRootSignature->createResourceGroup(1, 1, &samplerStateResource);
		}

		{ // Create resource group
			Rhi::IResource* resources[2] = { mVertexShaderUniformBuffer, mTexture2D };
			Rhi::ISamplerState* samplerStates[2] = { nullptr, static_cast<Rhi::ISamplerState*>(samplerStateResource) };
			mResourceGroup = mRootSignature->createResourceGroup(0, static_cast<uint32_t>(GLM_COUNTOF(resources)), resources, samplerStates);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
