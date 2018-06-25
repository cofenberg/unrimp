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
#include "RendererRuntime/DebugGui/DebugGuiManager.h"
#include "RendererRuntime/DebugGui/DebugGuiHelper.h"
#include "RendererRuntime/Resource/Texture/TextureResourceManager.h"
#include "RendererRuntime/Core/File/IFileManager.h"
#include "RendererRuntime/Core/IProfiler.h"
#include "RendererRuntime/IRendererRuntime.h"
#include "RendererRuntime/Context.h"

#include <imguizmo/ImGuizmo.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/detail/setup.hpp>	// For "glm::countof()"
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
		static constexpr Renderer::VertexAttribute VertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_2,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
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
				Renderer::VertexAttributeFormat::FLOAT_2,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
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
				Renderer::VertexAttributeFormat::R8G8B8A8_UNORM,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"Color",											// name[32] (char)
				"COLOR",											// semanticName[32] (char)
				0,													// semanticIndex (uint32_t)
				// Data source
				0,													// inputSlot (uint32_t)
				sizeof(float) * 4,									// alignedByteOffset (uint32_t)
				sizeof(float) * 4 + sizeof(uint8_t) * 4,			// strideInBytes (uint32_t)
				0													// instancesPerElement (uint32_t)
			}
		};
		const Renderer::VertexAttributes VertexAttributes(static_cast<uint32_t>(glm::countof(VertexAttributesLayout)), VertexAttributesLayout);


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void* AllocFunc(size_t sz, void* user_data)
		{
			return static_cast<Renderer::IAllocator*>(user_data)->reallocate(nullptr, 0, sz, 1);
		}

		void FreeFunc(void* ptr, void* user_data)
		{
			static_cast<Renderer::IAllocator*>(user_data)->reallocate(ptr, 0, 0, 1);
		}

		// From "imgui.cpp"
		ImGuiWindowSettings* FindWindowSettings(const char* name)
		{
			ImGuiContext& g = *GImGui;
			ImGuiID id = ImHash(name, 0);
			for (int i = 0; i != g.SettingsWindows.Size; i++)
			{
				ImGuiWindowSettings* ini = &g.SettingsWindows[i];
				if (ini->Id == id)
					return ini;
			}
			return NULL;
		}

		// From "imgui.cpp"
		ImGuiWindowSettings* AddWindowSettings(const char* name)
		{
			GImGui->SettingsWindows.resize(GImGui->SettingsWindows.Size + 1);
			ImGuiWindowSettings* ini = &GImGui->SettingsWindows.back();
			ini->Name = ImStrdup(name);
			ini->Id = ImHash(name, 0);
			ini->Collapsed = false;
			ini->Pos = ImVec2(FLT_MAX,FLT_MAX);
			ini->Size = ImVec2(0,0);
			return ini;
		}

		// From "imgui.cpp"
		void MarkIniSettingsDirty()
		{
			ImGuiContext& g = *GImGui;
			if (g.SettingsDirtyTimer <= 0.0f)
				g.SettingsDirtyTimer = g.IO.IniSavingRate;
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
	void DebugGuiManager::setImGuiAllocatorFunctions(Renderer::IAllocator& allocator)
	{
		ImGui::SetAllocatorFunctions(::detail::AllocFunc, ::detail::FreeFunc, &allocator);
	}

	void DebugGuiManager::getDefaultTextureAssetIds(AssetIds& assetIds)
	{
		// Define helper macro
		#define ADD_ASSET_ID(name) assetIds.push_back(STRING_ID(name));

		// Add asset IDs
		ADD_ASSET_ID("Unrimp/Texture/DynamicByCode/ImGuiGlyphMap2D")

		// Undefine helper macro
		#undef ADD_ASSET_ID
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void DebugGuiManager::newFrame(Renderer::IRenderTarget& renderTarget, CompositorWorkspaceInstance* compositorWorkspaceInstance)
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
			const bool previousOpenMetricsWindow = mOpenMetricsWindow;
			DebugGuiHelper::drawMetricsWindow(mOpenMetricsWindow, compositorWorkspaceInstance);
			if (previousOpenMetricsWindow != mOpenMetricsWindow)
			{
				setOpenMetricsWindowIniSetting();
			}
		}
	}

	Renderer::IVertexArrayPtr DebugGuiManager::getFillVertexArrayPtr()
	{
		if (GImGui->Initialized)
		{
			// Ask ImGui to render into the internal command buffer and then request the resulting draw data
			ImGui::Render();
			const ImDrawData* imDrawData = ImGui::GetDrawData();
			Renderer::IRenderer& renderer = mRendererRuntime.getRenderer();
			Renderer::IBufferManager& bufferManager = mRendererRuntime.getBufferManager();

			{ // Vertex and index buffers
				// Create and grow vertex/index buffers if needed
				if (nullptr == mVertexBufferPtr || mNumberOfAllocatedVertices < static_cast<uint32_t>(imDrawData->TotalVtxCount))
				{
					mNumberOfAllocatedVertices = static_cast<uint32_t>(imDrawData->TotalVtxCount + 5000);	// Add some reserve to reduce reallocations
					mVertexBufferPtr = bufferManager.createVertexBuffer(mNumberOfAllocatedVertices * sizeof(ImDrawVert), nullptr, Renderer::BufferUsage::DYNAMIC_DRAW);
					RENDERER_SET_RESOURCE_DEBUG_NAME(mVertexBufferPtr, "Debug GUI")
					mVertexArrayPtr = nullptr;
				}
				if (nullptr == mIndexBufferPtr || mNumberOfAllocatedIndices < static_cast<uint32_t>(imDrawData->TotalIdxCount))
				{
					mNumberOfAllocatedIndices = static_cast<uint32_t>(imDrawData->TotalIdxCount + 10000);	// Add some reserve to reduce reallocations
					mIndexBufferPtr = bufferManager.createIndexBuffer(mNumberOfAllocatedIndices * sizeof(ImDrawIdx), Renderer::IndexBufferFormat::UNSIGNED_SHORT, nullptr, Renderer::BufferUsage::DYNAMIC_DRAW);
					RENDERER_SET_RESOURCE_DEBUG_NAME(mIndexBufferPtr, "Debug GUI")
					mVertexArrayPtr = nullptr;
				}
				if (nullptr == mVertexArrayPtr)
				{
					assert(nullptr != mVertexBufferPtr);
					assert(nullptr != mIndexBufferPtr);

					// Create vertex array object (VAO)
					const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { mVertexBufferPtr };
					mVertexArrayPtr = bufferManager.createVertexArray(::detail::VertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, mIndexBufferPtr);
					RENDERER_SET_RESOURCE_DEBUG_NAME(mVertexArrayPtr, "Debug GUI")
				}

				{ // Copy and convert all vertices and indices into a single contiguous buffer
					Renderer::MappedSubresource vertexBufferMappedSubresource;
					if (renderer.map(*mVertexBufferPtr, 0, Renderer::MapType::WRITE_DISCARD, 0, vertexBufferMappedSubresource))
					{
						Renderer::MappedSubresource indexBufferMappedSubresource;
						if (renderer.map(*mIndexBufferPtr, 0, Renderer::MapType::WRITE_DISCARD, 0, indexBufferMappedSubresource))
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
							renderer.unmap(*mIndexBufferPtr, 0);
						}

						// Unmap the vertex buffer
						renderer.unmap(*mVertexBufferPtr, 0);
					}
				}
			}
		}

		// Done
		return mVertexArrayPtr;
	}

	void DebugGuiManager::fillCommandBuffer(Renderer::CommandBuffer& commandBuffer)
	{
		if (GImGui->Initialized)
		{
			// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
			RENDERER_SCOPED_PROFILER_EVENT_FUNCTION(mRendererRuntime.getContext(), commandBuffer)

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
						// Set scissor rectangle
						Renderer::Command::SetScissorRectangles::create(commandBuffer, static_cast<long>(pcmd->ClipRect.x), static_cast<long>(pcmd->ClipRect.y), static_cast<long>(pcmd->ClipRect.z), static_cast<long>(pcmd->ClipRect.w));

						// Draw
						Renderer::Command::DrawIndexed::create(commandBuffer, static_cast<uint32_t>(pcmd->ElemCount), 1, static_cast<uint32_t>(indexOffset), static_cast<int32_t>(vertexOffset));
					}
					indexOffset += pcmd->ElemCount;
				}
				vertexOffset += imDrawList->VtxBuffer.size();
			}
		}
	}

	void DebugGuiManager::fillCommandBufferUsingFixedBuildInRendererConfiguration(Renderer::CommandBuffer& commandBuffer)
	{
		if (GImGui->Initialized)
		{
			// Combined scoped profiler CPU and GPU sample as well as renderer debug event command
			RENDERER_SCOPED_PROFILER_EVENT_FUNCTION(mRendererRuntime.getContext(), commandBuffer)

			// Create fixed build in renderer configuration resources, if required
			if (nullptr == mRootSignature)
			{
				createFixedBuildInRendererConfigurationResources();
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
					Renderer::MappedSubresource mappedSubresource;
					Renderer::IRenderer& renderer = mRendererRuntime.getRenderer();
					if (renderer.map(*mVertexShaderUniformBuffer, 0, Renderer::MapType::WRITE_DISCARD, 0, mappedSubresource))
					{
						memcpy(mappedSubresource.data, objectSpaceToClipSpaceMatrix, sizeof(objectSpaceToClipSpaceMatrix));
						renderer.unmap(*mVertexShaderUniformBuffer, 0);
					}
				}
				else
				{
					// TODO(co) Not compatible with command buffer: This certainly is going to be removed, we need to implement internal uniform buffer emulation
					mProgram->setUniformMatrix4fv(mObjectSpaceToClipSpaceMatrixUniformHandle, &objectSpaceToClipSpaceMatrix[0][0]);
				}
			}

			{ // Renderer configuration
				// Set the used graphics root signature
				Renderer::Command::SetGraphicsRootSignature::create(commandBuffer, mRootSignature);

				// Set the used pipeline state object (PSO)
				Renderer::Command::SetPipelineState::create(commandBuffer, mPipelineState);

				// Set resource groups
				Renderer::Command::SetGraphicsResourceGroup::create(commandBuffer, 0, mResourceGroup);
				Renderer::Command::SetGraphicsResourceGroup::create(commandBuffer, 1, mSamplerStateGroup);
			}

			// Setup input assembly (IA): Set the used vertex array
			Renderer::Command::SetVertexArray::create(commandBuffer, getFillVertexArrayPtr());

			// Render command lists
			fillCommandBuffer(commandBuffer);
		}
	}

	bool DebugGuiManager::getIniSetting(const char* name, float value[4])
	{
		ImGuiWindowSettings* settings = ::detail::FindWindowSettings(name);
		if (nullptr != settings)
		{
			value[0] = settings->Pos.x;
			value[1] = settings->Pos.y;
			value[2] = settings->Size.x;
			value[3] = settings->Size.y;

			// Done
			return true;
		}

		// Ini setting not found
		return false;
	}

	void DebugGuiManager::setIniSetting(const char* name, const float value[4])
	{
		ImGuiWindowSettings* settings = ::detail::FindWindowSettings(name);
		if (nullptr == settings)
		{
			settings = ::detail::AddWindowSettings(name);
		}
		if (settings->Pos.x != value[0] || settings->Pos.y != value[1] ||
			settings->Size.x != value[2] || settings->Size.y != value[3])
		{
			settings->Pos.x = value[0];
			settings->Pos.y = value[1];
			settings->Size.x = value[2];
			settings->Size.y = value[3];
			::detail::MarkIniSettingsDirty();
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::DebugGuiManager methods ]
	//[-------------------------------------------------------]
	void DebugGuiManager::startup()
	{
		assert(!mIsRunning);

		{ // Create texture instance
			// Build texture atlas
			unsigned char* pixels = nullptr;
			int width = 0;
			int height = 0;
			ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

			// Upload texture to renderer
			mTexture2D = mRendererRuntime.getTextureManager().createTexture2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), Renderer::TextureFormat::R8, pixels, Renderer::TextureFlag::GENERATE_MIPMAPS);
			RENDERER_SET_RESOURCE_DEBUG_NAME(mTexture2D, "Debug 2D GUI glyph texture atlas")

			// Tell the texture resource manager about our render target texture so it can be referenced inside e.g. compositor nodes
			mRendererRuntime.getTextureResourceManager().createTextureResourceByAssetId(STRING_ID("Unrimp/Texture/DynamicByCode/ImGuiGlyphMap2D"), *mTexture2D);
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
		if (nullptr != localDataMountPoint)
		{
			// TODO(sw) These files don't get read/written via an file interface -> can break on mobile devices
			// TODO(co) The file manager now works with virtual filenames, this might resolve the issue since the local data mount point is considered to map to a file location were the application is allowed to write
			const std::string virtualDebugGuiDirectoryName = std::string(localDataMountPoint) + "/DebugGui";
			if (fileManager.createDirectories(virtualDebugGuiDirectoryName.c_str()))
			{
				// ImGui has no file system abstraction and needs absolute filenames
				mAbsoluteIniFilename = fileManager.mapVirtualToAbsoluteFilename(IFileManager::FileMode::WRITE, (virtualDebugGuiDirectoryName + "/UnrimpDebugGuiLayout.ini").c_str());
				mAbsoluteLogFilename = fileManager.mapVirtualToAbsoluteFilename(IFileManager::FileMode::WRITE, (virtualDebugGuiDirectoryName + "/UnrimpDebugGuiLog.txt").c_str());
				imGuiIo.IniFilename = mAbsoluteIniFilename.c_str();
				imGuiIo.LogFilename = mAbsoluteLogFilename.c_str();
			}
		}

		// Setup ImGui style and explicitly load the settings at once
		// -> "imgui.cpp" -> "LoadIniSettingsFromDisk()" will clamp values, we don't want this
		{
			ImGuiStyle& imGuiStyle = ImGui::GetStyle();
			const ImVec2 windowMinSizeBackup = imGuiStyle.WindowMinSize;
			imGuiStyle.WindowMinSize.x = imGuiStyle.WindowMinSize.y = std::numeric_limits<float>::lowest();
			ImGui::StyleColorsDark();
			imGuiIo.LoadIniSettings();
			imGuiStyle.WindowMinSize = windowMinSizeBackup;
		}

		{ // Read ini-settings
			float value[4] = {};
			if (getIniSetting("OpenMetricsWindow", value))
			{
				mOpenMetricsWindow = (0 != value[0]);
			}
		}
	}

	DebugGuiManager::~DebugGuiManager()
	{
		ImGui::DestroyContext(mImGuiContext);
	}

	void DebugGuiManager::createFixedBuildInRendererConfigurationResources()
	{
		Renderer::IRenderer& renderer = mRendererRuntime.getRenderer();
		assert(nullptr == mRootSignature);

		{ // Create the root signature instance
			// Create the root signature
			Renderer::DescriptorRangeBuilder ranges[3];
			ranges[0].initialize(Renderer::DescriptorRangeType::UBV, 1, 0, "UniformBlockDynamicVs", Renderer::ShaderVisibility::VERTEX);
			ranges[1].initialize(Renderer::DescriptorRangeType::SRV, 1, 0, "GlyphMap", Renderer::ShaderVisibility::FRAGMENT);
			ranges[2].initializeSampler(1, 0, Renderer::ShaderVisibility::FRAGMENT);

			Renderer::RootParameterBuilder rootParameters[2];
			rootParameters[0].initializeAsDescriptorTable(2, &ranges[0]);
			rootParameters[1].initializeAsDescriptorTable(1, &ranges[2]);

			// Setup
			Renderer::RootSignatureBuilder rootSignature;
			rootSignature.initialize(static_cast<uint32_t>(glm::countof(rootParameters)), rootParameters, 0, nullptr, Renderer::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mRootSignature = renderer.createRootSignature(rootSignature);
			RENDERER_SET_RESOURCE_DEBUG_NAME(mRootSignature, "Debug GUI")
		}

		// Decide which shader language should be used (for example "GLSL" or "HLSL")
		Renderer::IShaderLanguage* shaderLanguage = renderer.getShaderLanguage();
		if (nullptr != shaderLanguage)
		{
			{ // Create the pipeline state instance
				{ // Create the program
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
					Renderer::IVertexShader* vertexShader = shaderLanguage->createVertexShaderFromSourceCode(::detail::VertexAttributes, vertexShaderSourceCode);
					RENDERER_SET_RESOURCE_DEBUG_NAME(vertexShader, "Debug GUI")
					Renderer::IFragmentShader* fragmentShader = shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode);
					RENDERER_SET_RESOURCE_DEBUG_NAME(fragmentShader, "Debug GUI")

					// Create the program
					mProgram = shaderLanguage->createProgram(
						*mRootSignature,
						::detail::VertexAttributes,
						vertexShader,
						fragmentShader);
					RENDERER_SET_RESOURCE_DEBUG_NAME(mProgram, "Debug GUI")
				}

				// Create the pipeline state object (PSO)
				if (nullptr != mProgram)
				{
					// TODO(co) Render pass related update, the render pass in here is currently just a dummy so the debug compositor works
					Renderer::IRenderPass* renderPass = renderer.createRenderPass(1, &renderer.getCapabilities().preferredSwapChainColorTextureFormat, renderer.getCapabilities().preferredSwapChainDepthStencilTextureFormat);

					Renderer::PipelineState pipelineState = Renderer::PipelineStateBuilder(mRootSignature, mProgram, ::detail::VertexAttributes, *renderPass);
					pipelineState.rasterizerState.cullMode				   = Renderer::CullMode::NONE;
					pipelineState.rasterizerState.scissorEnable			   = 1;
					pipelineState.depthStencilState.depthEnable			   = false;
					pipelineState.depthStencilState.depthWriteMask		   = Renderer::DepthWriteMask::ZERO;
					pipelineState.blendState.renderTarget[0].blendEnable   = true;
					pipelineState.blendState.renderTarget[0].srcBlend	   = Renderer::Blend::SRC_ALPHA;
					pipelineState.blendState.renderTarget[0].destBlend	   = Renderer::Blend::INV_SRC_ALPHA;
					pipelineState.blendState.renderTarget[0].srcBlendAlpha = Renderer::Blend::INV_SRC_ALPHA;
					mPipelineState = renderer.createPipelineState(pipelineState);
					RENDERER_SET_RESOURCE_DEBUG_NAME(mPipelineState, "Debug GUI")
				}
			}
		}

		// Create vertex uniform buffer instance
		if (renderer.getCapabilities().maximumUniformBufferSize > 0)
		{
			mVertexShaderUniformBuffer = mRendererRuntime.getBufferManager().createUniformBuffer(sizeof(float) * 4 * 4, nullptr, Renderer::BufferUsage::DYNAMIC_DRAW);
			RENDERER_SET_RESOURCE_DEBUG_NAME(mVertexShaderUniformBuffer, "Debug GUI")
		}
		else if (nullptr != mProgram)
		{
			mObjectSpaceToClipSpaceMatrixUniformHandle = mProgram->getUniformHandle("ObjectSpaceToClipSpaceMatrix");
		}

		// Create sampler state instance and wrap it into a resource group instance
		Renderer::IResource* samplerStateResource = nullptr;
		{
			Renderer::SamplerState samplerState = Renderer::ISamplerState::getDefaultSamplerState();
			samplerState.addressU = Renderer::TextureAddressMode::WRAP;
			samplerState.addressV = Renderer::TextureAddressMode::WRAP;
			samplerStateResource = renderer.createSamplerState(samplerState);
			RENDERER_SET_RESOURCE_DEBUG_NAME(samplerStateResource, "Debug GUI")
			mSamplerStateGroup = mRootSignature->createResourceGroup(1, 1, &samplerStateResource);
		}

		{ // Create resource group
			Renderer::IResource* resources[2] = { mVertexShaderUniformBuffer, mTexture2D };
			Renderer::ISamplerState* samplerStates[2] = { nullptr, static_cast<Renderer::ISamplerState*>(samplerStateResource) };
			mResourceGroup = mRootSignature->createResourceGroup(0, static_cast<uint32_t>(glm::countof(resources)), resources, samplerStates);
		}
	}

	void DebugGuiManager::setOpenMetricsWindowIniSetting()
	{
		// Update ini-settings
		const float value[4] = { mOpenMetricsWindow ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
		setIniSetting("OpenMetricsWindow", value);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
