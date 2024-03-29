/*********************************************************\
 * Copyright (c) 2012-2022 The Unrimp Team
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
#include "Renderer/Public/DebugGui/DebugGuiManager.h"
#include "Renderer/Public/DebugGui/DebugGuiHelper.h"
#include "Renderer/Public/Resource/Texture/TextureResourceManager.h"
#include "Renderer/Public/Core/File/IFileManager.h"
#include "Renderer/Public/Core/IProfiler.h"
#include "Renderer/Public/IRenderer.h"
#include "Renderer/Public/Context.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <ImGuizmo/ImGuizmo.h>

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
		static const Renderer::AssetId IMGUI_GLYPH_MAP_2D(ASSET_ID("Unrimp/Texture/DynamicByCode/ImGuiGlyphMap2D"));

		// Vertex input layout
		static constexpr Rhi::VertexAttribute VertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Rhi::VertexAttributeFormat::FLOAT_4,		// vertexAttributeFormat (Rhi::VertexAttributeFormat)
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
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		static const Renderer::IFileManager* g_FileManager = nullptr;


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
//[ Global functions                                      ]
//[-------------------------------------------------------]
#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
	ImFileHandle ImFileOpen(const char* filename, const char* mode)
	{
		// Get the file mode
		Renderer::IFileManager::FileMode fileMode = Renderer::IFileManager::FileMode::READ;
		if (::stricmp(mode, "rb") == 0)
		{
			fileMode = Renderer::IFileManager::FileMode::READ;
		}
		else if (::stricmp(mode, "wt") == 0)
		{
			fileMode = Renderer::IFileManager::FileMode::WRITE;
		}
		else
		{
			// Error, unsupported file mode!
			return static_cast<ImFileHandle>(0);
		}

		// Open the file
		return static_cast<ImFileHandle>(::detail::g_FileManager->openFile(fileMode, filename));
	}

	bool ImFileClose(ImFileHandle file)
	{
		if (nullptr != file)
		{
			::detail::g_FileManager->closeFile(*reinterpret_cast<Renderer::IFile*>(file));

			// Done
			return true;
		}
		else
		{
			// Error!
			return false;
		}
	}

	ImU64 ImFileGetSize(ImFileHandle file)
	{
		return (nullptr != file) ? reinterpret_cast<Renderer::IFile*>(file)->getNumberOfBytes() : static_cast<ImU64>(-1);	// -1 = Value of the empty ImGui file implementation
	}

	ImU64 ImFileRead(void* data, ImU64 size, ImU64 count, ImFileHandle file)
	{
		reinterpret_cast<Renderer::IFile*>(file)->read(data, static_cast<size_t>(size * count));
		return count;
	}

	ImU64 ImFileWrite(const void* data, ImU64 size, ImU64 count, ImFileHandle file)
	{
		reinterpret_cast<Renderer::IFile*>(file)->write(data, static_cast<size_t>(size * count));
		return count;
	}
#endif


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
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
		assetIds.push_back(::detail::IMGUI_GLYPH_MAP_2D);
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
			Rhi::IRhi& rhi = mRenderer.getRhi();
			Rhi::IBufferManager& bufferManager = mRenderer.getBufferManager();

			{ // Vertex and index buffers
				// Create and grow vertex/index buffers if needed
				if (nullptr == mVertexBuffer || mNumberOfAllocatedVertices < static_cast<uint32_t>(imDrawData->TotalVtxCount))
				{
					mNumberOfAllocatedVertices = static_cast<uint32_t>(imDrawData->TotalVtxCount + 5000);	// Add some reserve to reduce reallocations
					mVertexBuffer = bufferManager.createVertexBuffer(mNumberOfAllocatedVertices * sizeof(ImDrawVert), nullptr, 0, Rhi::BufferUsage::DYNAMIC_DRAW RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
					mVertexArray = nullptr;
				}
				if (nullptr == mIndexBuffer || mNumberOfAllocatedIndices < static_cast<uint32_t>(imDrawData->TotalIdxCount))
				{
					mNumberOfAllocatedIndices = static_cast<uint32_t>(imDrawData->TotalIdxCount + 10000);	// Add some reserve to reduce reallocations
					mIndexBuffer = bufferManager.createIndexBuffer(mNumberOfAllocatedIndices * sizeof(ImDrawIdx), nullptr, 0, Rhi::BufferUsage::DYNAMIC_DRAW, Rhi::IndexBufferFormat::UNSIGNED_SHORT RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
					mVertexArray = nullptr;
				}
				if (nullptr == mVertexArray)
				{
					RHI_ASSERT(mRenderer.getContext(), nullptr != mVertexBuffer, "Invalid vertex buffer")
					RHI_ASSERT(mRenderer.getContext(), nullptr != mIndexBuffer, "Invalid index buffer")

					// Create vertex array object (VAO)
					const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { { mVertexBuffer } };
					mVertexArray = bufferManager.createVertexArray(::detail::VertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, mIndexBuffer RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
				}

				{ // Copy and convert all vertices and indices into a single contiguous buffer
					Rhi::MappedSubresource vertexBufferMappedSubresource;
					if (rhi.map(*mVertexBuffer, 0, Rhi::MapType::WRITE_DISCARD, 0, vertexBufferMappedSubresource))
					{
						Rhi::MappedSubresource indexBufferMappedSubresource;
						if (rhi.map(*mIndexBuffer, 0, Rhi::MapType::WRITE_DISCARD, 0, indexBufferMappedSubresource))
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
							rhi.unmap(*mIndexBuffer, 0);
						}

						// Unmap the vertex buffer
						rhi.unmap(*mVertexBuffer, 0);
					}
				}
			}
		}

		// Done
		return mVertexArray;
	}

	void DebugGuiManager::fillGraphicsCommandBuffer(Rhi::CommandBuffer& commandBuffer)
	{
		if (GImGui->Initialized)
		{
			// No combined scoped profiler CPU and GPU sample as well as renderer debug event command by intent, this is something the caller has to take care of
			// RENDERER_SCOPED_PROFILER_EVENT(mRenderer.getContext(), commandBuffer, "Debug GUI")

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
			// RENDERER_SCOPED_PROFILER_EVENT(mRenderer.getContext(), commandBuffer, "Fixed debug GUI")

			// Create fixed build in RHI configuration resources, if required
			if (nullptr == mRootSignature)
			{
				createFixedBuildInRhiConfigurationResources();
			}

			{ // Setup orthographic projection matrix into our vertex shader uniform buffer
				const ImVec2& displaySize = ImGui::GetIO().DisplaySize;
				ImVec2 texelOffset(0.0f, 0.0f);
				if (mRenderer.getRhi().getNameId() == Rhi::NameId::DIRECT3D9)
				{
					// Take care of the Direct3D 9 half-pixel/half-texel offset
					// -> See "Directly Mapping Texels to Pixels (Direct3D 9)" at https://docs.microsoft.com/en-gb/windows/win32/direct3d9/directly-mapping-texels-to-pixels?redirectedfrom=MSDN
					texelOffset.x += 0.5f;
					texelOffset.y += 0.5f;
				}
				const float l = texelOffset.x;
				const float r = displaySize.x + texelOffset.x;
				const float t = texelOffset.y;
				const float b = displaySize.y + texelOffset.y;
				const float objectSpaceToClipSpaceMatrix[4][4] =
				{
					{ 2.0f / (r - l),	 0.0f,        0.0f, 0.0f },
					{ 0.0f,				 2.0f/(t-b),  0.0f, 0.0f },
					{ 0.0f,				 0.0f,        0.5f, 0.0f },
					{ (r + l) / (l - r), (t+b)/(b-t), 0.5f, 1.0f }
				};
				if (nullptr != mVertexShaderUniformBuffer)
				{
					// Copy data into the uniform buffer
					Rhi::Command::CopyUniformBufferData::create(commandBuffer, *mVertexShaderUniformBuffer, objectSpaceToClipSpaceMatrix, sizeof(objectSpaceToClipSpaceMatrix));
				}
				else
				{
					// Set legacy uniforms
					Rhi::Command::SetUniform::createMatrix4fv(commandBuffer, *mGraphicsProgram, mObjectSpaceToClipSpaceMatrixUniformHandle, &objectSpaceToClipSpaceMatrix[0][0]);
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
	//[ Protected virtual Renderer::DebugGuiManager methods   ]
	//[-------------------------------------------------------]
	void DebugGuiManager::startup()
	{
		RHI_ASSERT(mRenderer.getContext(), !mIsRunning, "The debug GUI manager is already running")

		{ // Create texture instance
			// Build texture atlas
			unsigned char* pixels = nullptr;
			int width = 0;
			int height = 0;
			ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

			// Upload texture to RHI
			mTexture2D = mRenderer.getTextureManager().createTexture2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), Rhi::TextureFormat::R8, pixels, Rhi::TextureFlag::GENERATE_MIPMAPS | Rhi::TextureFlag::SHADER_RESOURCE, Rhi::TextureUsage::DEFAULT, 1, nullptr RHI_RESOURCE_DEBUG_NAME("Debug 2D GUI glyph texture atlas"));

			// Tell the texture resource manager about our glyph texture so it can be referenced inside e.g. compositor nodes
			mRenderer.getTextureResourceManager().createTextureResourceByAssetId(::detail::IMGUI_GLYPH_MAP_2D, *mTexture2D);
		}
	}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	DebugGuiManager::DebugGuiManager(IRenderer& renderer) :
		mRenderer(renderer),
		mImGuiContext(nullptr),
		mIsRunning(false),
		mObjectSpaceToClipSpaceMatrixUniformHandle(NULL_HANDLE),
		mNumberOfAllocatedVertices(0),
		mNumberOfAllocatedIndices(0),
		mOpenMetricsWindow(false)
	{
		// Set ImGui allocator functions
		setImGuiAllocatorFunctions(renderer.getContext().getAllocator());

		// Create ImGui context
		mImGuiContext = ImGui::CreateContext();

		// Change ImGui filenames so one is able to guess where those files come from when using Unrimp
		const IFileManager& fileManager = renderer.getFileManager();
		::detail::g_FileManager = &fileManager;
		const char* localDataMountPoint = fileManager.getLocalDataMountPoint();
		ImGuiIO& imGuiIo = ImGui::GetIO();
		imGuiIo.IniFilename = nullptr;
		imGuiIo.LogFilename = nullptr;
		if (nullptr != localDataMountPoint && fileManager.createDirectories(localDataMountPoint))
		{
			const std::string virtualDebugGuiDirectoryName = localDataMountPoint;
			mVirtualIniFilename = (virtualDebugGuiDirectoryName + "/UnrimpImGuiLayout.ini").c_str();
			mVirtualLogFilename = (virtualDebugGuiDirectoryName + "/UnrimpImGuiLog.txt").c_str();
			imGuiIo.IniFilename = mVirtualIniFilename.c_str();
			imGuiIo.LogFilename = mVirtualLogFilename.c_str();
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
		Rhi::IRhi& rhi = mRenderer.getRhi();
		RHI_ASSERT(mRenderer.getContext(), nullptr == mRootSignature, "The debug GUI manager has already root signature")

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
			Rhi::RootSignatureBuilder rootSignatureBuilder;
			rootSignatureBuilder.initialize(static_cast<uint32_t>(GLM_COUNTOF(rootParameters)), rootParameters, 0, nullptr, Rhi::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mRootSignature = rhi.createRootSignature(rootSignatureBuilder RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
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

				// Create the graphics program
				Rhi::IShaderLanguage& shaderLanguage = rhi.getDefaultShaderLanguage();
				mGraphicsProgram = shaderLanguage.createGraphicsProgram(
					*mRootSignature,
					::detail::VertexAttributes,
					shaderLanguage.createVertexShaderFromSourceCode(::detail::VertexAttributes, vertexShaderSourceCode, nullptr RHI_RESOURCE_DEBUG_NAME("Debug GUI")),
					shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode, nullptr RHI_RESOURCE_DEBUG_NAME("Debug GUI"))
					RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
			}

			// Create the graphics pipeline state object (PSO)
			if (nullptr != mGraphicsProgram)
			{
				// TODO(co) Render pass related update, the render pass in here is currently just a dummy so the debug compositor works
				Rhi::IRenderPass* renderPass = rhi.createRenderPass(1, &rhi.getCapabilities().preferredSwapChainColorTextureFormat, rhi.getCapabilities().preferredSwapChainDepthStencilTextureFormat, 1 RHI_RESOURCE_DEBUG_NAME("Debug GUI"));

				Rhi::GraphicsPipelineState graphicsPipelineState = Rhi::GraphicsPipelineStateBuilder(mRootSignature, mGraphicsProgram, ::detail::VertexAttributes, *renderPass);
				graphicsPipelineState.rasterizerState.cullMode				 = Rhi::CullMode::NONE;
				graphicsPipelineState.rasterizerState.scissorEnable			 = 1;
				graphicsPipelineState.depthStencilState.depthEnable			 = false;
				graphicsPipelineState.depthStencilState.depthWriteMask		 = Rhi::DepthWriteMask::ZERO;
				graphicsPipelineState.blendState.renderTarget[0].blendEnable = true;
				graphicsPipelineState.blendState.renderTarget[0].srcBlend	 = Rhi::Blend::SRC_ALPHA;
				graphicsPipelineState.blendState.renderTarget[0].destBlend	 = Rhi::Blend::INV_SRC_ALPHA;
				mGraphicsPipelineState = rhi.createGraphicsPipelineState(graphicsPipelineState RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
			}
		}

		// Create vertex uniform buffer instance
		if (rhi.getCapabilities().maximumUniformBufferSize > 0)
		{
			mVertexShaderUniformBuffer = mRenderer.getBufferManager().createUniformBuffer(sizeof(float) * 4 * 4, nullptr, Rhi::BufferUsage::DYNAMIC_DRAW RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
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
			samplerStateResource = rhi.createSamplerState(samplerState RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
			mSamplerStateGroup = mRootSignature->createResourceGroup(1, 1, &samplerStateResource, nullptr RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
		}

		{ // Create resource group
			Rhi::IResource* resources[2] = { mVertexShaderUniformBuffer, mTexture2D };
			Rhi::ISamplerState* samplerStates[2] = { nullptr, static_cast<Rhi::ISamplerState*>(samplerStateResource) };
			mResourceGroup = mRootSignature->createResourceGroup(0, static_cast<uint32_t>(GLM_COUNTOF(resources)), resources, samplerStates RHI_RESOURCE_DEBUG_NAME("Debug GUI"));
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
