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
#include <Rhi/Public/DefaultLog.h>
#include <Rhi/Public/DefaultAssert.h>
#include <Rhi/Public/DefaultAllocator.h>
#include <Rhi/Public/RhiInstance.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '__GNUC__' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <SDL.h>
	#include <SDL_syswm.h>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Global variables                                      ]
//[-------------------------------------------------------]
extern "C"
{
	// NVIDIA: Force usage of NVidia GPU in case there is an integrated graphics unit as well, if we don't do this we risk getting the integrated graphics unit and hence a horrible performance
	// -> See "Enabling High Performance Graphics Rendering on Optimus Systems" http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

	// AMD: Force usage of AMD GPU in case there is an integrated graphics unit as well, if we don't do this we risk getting the integrated graphics unit and hence a horrible performance
	// -> Named "Dynamic Switchable Graphics", found no official documentation, only https://community.amd.com/message/1307599#comment-1307599 - "Can an OpenGL app default to the discrete GPU on an Enduro system?"
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}


//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
Rhi::handle getNativeWindowHandle(SDL_Window& sdlWindow)
{
	Rhi::handle nativeWindowHandle = NULL_HANDLE;
	SDL_SysWMinfo sdlSysWMinfo;
	SDL_VERSION(&sdlSysWMinfo.version);
	if (SDL_GetWindowWMInfo(&sdlWindow, &sdlSysWMinfo))
	{
		switch (sdlSysWMinfo.subsystem)
		{
			#if defined _WIN32
				case SDL_SYSWM_UNKNOWN:
					ASSERT(false);
					break;

				case SDL_SYSWM_WINDOWS:
					nativeWindowHandle = reinterpret_cast<Rhi::handle>(sdlSysWMinfo.info.win.window);
					break;

				case SDL_SYSWM_X11:
				case SDL_SYSWM_DIRECTFB:
				case SDL_SYSWM_COCOA:
				case SDL_SYSWM_UIKIT:
				case SDL_SYSWM_WAYLAND:
				case SDL_SYSWM_MIR:
				case SDL_SYSWM_WINRT:
				case SDL_SYSWM_ANDROID:
				case SDL_SYSWM_VIVANTE:
				case SDL_SYSWM_OS2:
					ASSERT(false);
					break;
			#elif defined __ANDROID__
				#warning "TODO(co) The Android support is work-in-progress"
			#elif defined LINUX
				case SDL_SYSWM_UNKNOWN:
				case SDL_SYSWM_WINDOWS:
					ASSERT(false);
					break;

				case SDL_SYSWM_X11:
					nativeWindowHandle = reinterpret_cast<Rhi::handle>(sdlSysWMinfo.info.x11.window);
					break;

				case SDL_SYSWM_DIRECTFB:
				case SDL_SYSWM_COCOA:
				case SDL_SYSWM_UIKIT:
					ASSERT(false);
					break;

				case SDL_SYSWM_WAYLAND:
					nativeWindowHandle = reinterpret_cast<Rhi::handle>(sdlSysWMinfo.info.wl.surface);
					break;

				case SDL_SYSWM_MIR:
				case SDL_SYSWM_WINRT:
				case SDL_SYSWM_ANDROID:
				case SDL_SYSWM_VIVANTE:
				case SDL_SYSWM_OS2:
					ASSERT(false);
					break;
			#else
				#error "Unsupported platform"
			#endif
		}
	}

	// Done
	return nativeWindowHandle;
}


//[-------------------------------------------------------]
//[ Platform independent program entry point              ]
//[-------------------------------------------------------]
int main(int arc, char* argv[])
{
	// For memory leak detection
	#if defined(RHI_DEBUG) && defined(_WIN32)
		// "_CrtDumpMemoryLeaks()" reports false positive memory leak with static variables, so use a memory difference instead
		_CrtMemState crtMemState = { };
		_CrtMemCheckpoint(&crtMemState);
	#endif

	// Initialize SDL 2
	if (SDL_Init(0) == 0)
	{
		// Create SDL 2 window instance
		SDL_Window* sdlWindow = SDL_CreateWindow("Example SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		if (nullptr != sdlWindow)
		{
			// Create RHI instance
			Rhi::DefaultLog defaultLog;
			Rhi::DefaultAssert defaultAssert;
			Rhi::DefaultAllocator defaultAllocator;
			#ifdef _WIN32
				Rhi::Context rhiContext(defaultLog, defaultAssert, defaultAllocator, getNativeWindowHandle(*sdlWindow));
				const bool loadRhiApiSharedLibrary = false;
			#elif LINUX
				// Under Linux the OpenGL library interacts with the library from X11 so we need to load the library ourself instead letting it be loaded by the RHI instance
				// -> See http://dri.sourceforge.net/doc/DRIuserguide.html "11.5 libGL.so and dlopen()"
				const bool loadRhiApiSharedLibrary = true;
				Rhi::X11Context rhiContext(defaultLog, defaultAssert, defaultAllocator, getX11Display(), getNativeWindowHandle(*sdlWindow));
			#endif
			#ifdef RHI_DIRECT3D11
				const char* defaultRhiName = "Direct3D11";
			#elif defined(RHI_OPENGL)
				const char* defaultRhiName = "OpenGL";
			#elif defined(RHI_DIRECT3D10)
				const char* defaultRhiName = "Direct3D10";
			#elif defined(RHI_DIRECT3D9)
				const char* defaultRhiName = "Direct3D9";
			#elif defined(RHI_OPENGLES3)
				const char* defaultRhiName = "OpenGLES3";
			#elif defined(RHI_VULKAN)
				const char* defaultRhiName = "Vulkan";
			#elif defined(RHI_DIRECT3D12)
				const char* defaultRhiName = "Direct3D12";
			#elif defined(RHI_NULL)
				const char* defaultRhiName = "Null";
			#endif
			Rhi::RhiInstance rhiInstance((arc > 1) ? argv[1] : defaultRhiName, rhiContext, loadRhiApiSharedLibrary);
			Rhi::IRhiPtr rhi = rhiInstance.getRhi();
			if (nullptr != rhi && rhi->isInitialized())
			{
				// Create RHI resources
				Rhi::ISwapChainPtr mainSwapChain;
				Rhi::IBufferManagerPtr bufferManager;
				Rhi::IRootSignaturePtr rootSignature;
				Rhi::IGraphicsPipelineStatePtr graphicsPipelineState;
				Rhi::IVertexArrayPtr vertexArray;
				{
					{ // Create RHI swap chain instance
						const Rhi::Capabilities& capabilities = rhi->getCapabilities();
						mainSwapChain = rhi->createSwapChain(
							*rhi->createRenderPass(1, &capabilities.preferredSwapChainColorTextureFormat, capabilities.preferredSwapChainDepthStencilTextureFormat, 1 RHI_RESOURCE_DEBUG_NAME("Main")),
							Rhi::WindowHandle{getNativeWindowHandle(*sdlWindow), nullptr, nullptr},	// TODO(co) Linux Wayland support
							rhi->getContext().isUsingExternalContext()
							RHI_RESOURCE_DEBUG_NAME("Main"));
					}

					// Create the buffer manager
					bufferManager = rhi->createBufferManager();

					{ // Create the root signature
						// Setup
						Rhi::RootSignatureBuilder rootSignatureBuilder;
						rootSignatureBuilder.initialize(0, nullptr, 0, nullptr, Rhi::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

						// Create the instance
						rootSignature = rhi->createRootSignature(rootSignatureBuilder RHI_RESOURCE_DEBUG_NAME("Triangle"));
					}

					// Vertex input layout
					static constexpr Rhi::VertexAttribute vertexAttributesLayout[] =
					{
						{ // Attribute 0
							// Data destination
							Rhi::VertexAttributeFormat::FLOAT_2,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
							"Position",								// name[32] (char)
							"POSITION",								// semanticName[32] (char)
							0,										// semanticIndex (uint32_t)
							// Data source
							0,										// inputSlot (uint32_t)
							0,										// alignedByteOffset (uint32_t)
							sizeof(float) * 2,						// strideInBytes (uint32_t)
							0										// instancesPerElement (uint32_t)
						}
					};
					const Rhi::VertexAttributes vertexAttributes(sizeof(vertexAttributesLayout) / sizeof(Rhi::VertexAttribute), vertexAttributesLayout);

					{ // Create vertex array object (VAO)
						// Create the vertex buffer object (VBO)
						// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
						static constexpr float VERTEX_POSITION[] =
						{					// Vertex ID	Triangle on screen
							 0.0f, 1.0f,	// 0				0
							 1.0f, 0.0f,	// 1			   .   .
							-0.5f, 0.0f		// 2			  2.......1
						};
						Rhi::IVertexBufferPtr vertexBuffer(bufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION, 0, Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME("Triangle")));

						// Create vertex array object (VAO)
						// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
						// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
						// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
						//    reference of the used vertex buffer objects (VBO). If the reference counter of a
						//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
						const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
						vertexArray = bufferManager->createVertexArray(vertexAttributes, sizeof(vertexArrayVertexBuffers) / sizeof(Rhi::VertexArrayVertexBuffer), vertexArrayVertexBuffers, nullptr RHI_RESOURCE_DEBUG_NAME("Triangle"));
					}

					{
						// Create the graphics program
						Rhi::IGraphicsProgramPtr graphicsProgram;
						{
							// Get the shader source code (outsourced to keep an overview)
							const char* vertexShaderSourceCode = nullptr;
							const char* fragmentShaderSourceCode = nullptr;
							#include "ExampleSDL2_GLSL_450.h"	// For Vulkan
							#include "ExampleSDL2_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
							#include "ExampleSDL2_GLSL_ES3.h"
							#include "ExampleSDL2_HLSL_D3D9_D3D10_D3D11_D3D12.h"
							#include "ExampleSDL2_Null.h"

							// Create the graphics program
							Rhi::IShaderLanguage& shaderLanguage = rhi->getDefaultShaderLanguage();
							graphicsProgram = shaderLanguage.createGraphicsProgram(
								*rootSignature,
								vertexAttributes,
								shaderLanguage.createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode, nullptr RHI_RESOURCE_DEBUG_NAME("Triangle")),
								shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode, nullptr RHI_RESOURCE_DEBUG_NAME("Triangle"))
								RHI_RESOURCE_DEBUG_NAME("Triangle"));
						}

						// Create the graphics pipeline state object (PSO)
						if (nullptr != graphicsProgram)
						{
							graphicsPipelineState = rhi->createGraphicsPipelineState(Rhi::GraphicsPipelineStateBuilder(rootSignature, graphicsProgram, vertexAttributes, mainSwapChain->getRenderPass()) RHI_RESOURCE_DEBUG_NAME("Triangle"));
						}
					}
				}

				// Record RHI command buffer
				Rhi::CommandBuffer commandBuffer;
				{
					// Scoped debug event
					COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(commandBuffer)

					// Make the graphics main swap chain to the current render target
					Rhi::Command::SetGraphicsRenderTarget::create(commandBuffer, mainSwapChain);

					{ // Since Direct3D 12 is command list based, the viewport and scissor rectangle must be set in every draw call to work with all supported RHI implementations
						// Get the window size
						uint32_t width  = 1;
						uint32_t height = 1;
						mainSwapChain->getWidthAndHeight(width, height);

						// Set the graphics viewport and scissor rectangle
						Rhi::Command::SetGraphicsViewportAndScissorRectangle::create(commandBuffer, 0, 0, width, height);
					}

					{ // Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
						const float color[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
						Rhi::Command::ClearGraphics::create(commandBuffer, Rhi::ClearFlag::COLOR_DEPTH, color);
					}

					// Set the used graphics root signature
					Rhi::Command::SetGraphicsRootSignature::create(commandBuffer, rootSignature);

					// Set the used graphics pipeline state object (PSO)
					Rhi::Command::SetGraphicsPipelineState::create(commandBuffer, graphicsPipelineState);

					// Input assembly (IA): Set the used vertex array
					Rhi::Command::SetGraphicsVertexArray::create(commandBuffer, vertexArray);

					// Set debug marker
					// -> Debug methods: When using Direct3D <11.1, these methods map to the Direct3D 9 PIX functions
					//    (D3DPERF_* functions, also works directly within VisualStudio 2017 out-of-the-box)
					COMMAND_SET_DEBUG_MARKER(commandBuffer, "Everyone ready for the upcoming triangle?")

					{
						// Scoped debug event
						COMMAND_SCOPED_DEBUG_EVENT(commandBuffer, "Drawing the fancy triangle")

						// Render the specified geometric primitive, based on an array of vertices
						Rhi::Command::DrawGraphics::create(commandBuffer, 3);
					}
				}

				// Main loop
				bool quit = false;
				SDL_Event sdlEvent;
				while (!quit && SDL_WaitEvent(&sdlEvent) != 0)
				{
					switch (sdlEvent.type)
					{
						case SDL_QUIT:
							// Shut down the application
							quit = true;
							break;

						case SDL_WINDOWEVENT:
							switch (sdlEvent.window.event)
							{
								case SDL_WINDOWEVENT_EXPOSED:
									// Begin scene rendering
									if (rhi->beginScene())
									{
										// Submit command buffer to the RHI implementation
										commandBuffer.submitToRhi(*rhi);

										// End scene rendering
										rhi->endScene();
									}

									// Present the content of the current back buffer
									mainSwapChain->present();
									break;

								case SDL_WINDOWEVENT_SIZE_CHANGED:
									// Inform the swap chain that the size of the native window was changed
									// -> Required for Direct3D 9, Direct3D 10, Direct3D 11
									// -> Not required for OpenGL and OpenGL ES 3
									mainSwapChain->resizeBuffers();
									break;
							}
							break;

						case SDL_KEYDOWN:
							// Toggle the fullscreen state
							if (SDLK_RETURN == sdlEvent.key.keysym.sym && (sdlEvent.key.keysym.mod & KMOD_ALT) != 0)
							{
								mainSwapChain->setFullscreenState(!mainSwapChain->getFullscreenState());
							}
							break;
					}
				}
			}
			SDL_DestroyWindow(sdlWindow);
		}
		SDL_Quit();
	}

	// For memory leak detection
	#if defined(RHI_DEBUG) && defined(_WIN32)
		_CrtMemDumpAllObjectsSince(&crtMemState);
	#endif

	// No error
	return 0;
}
