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


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Examples/Private/Basics/FirstTexture/FirstTexture.h"
#include "Examples/Private/Framework/Color4.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP

#include <float.h> // For FLT_MAX
#include <stdlib.h> // For rand()


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstTexture::onInitialization()
{
	// Get and check the RHI instance
	Rhi::IRhiPtr rhi(getRhi());
	if (nullptr != rhi)
	{
		// Create the buffer and texture manager
		mBufferManager = rhi->createBufferManager();
		mTextureManager = rhi->createTextureManager();

		{ // Create the root signature
			Rhi::DescriptorRangeBuilder ranges[4];
			ranges[0].initialize(Rhi::ResourceType::TEXTURE_1D, 0, "GradientMap", Rhi::ShaderVisibility::FRAGMENT);
			ranges[1].initialize(Rhi::ResourceType::TEXTURE_2D, 1, "AlbedoMap",   Rhi::ShaderVisibility::FRAGMENT);
			ranges[2].initializeSampler(0, Rhi::ShaderVisibility::FRAGMENT);
			ranges[3].initializeSampler(1, Rhi::ShaderVisibility::FRAGMENT);

			Rhi::RootParameterBuilder rootParameters[2];
			rootParameters[0].initializeAsDescriptorTable(2, &ranges[0]);
			rootParameters[1].initializeAsDescriptorTable(2, &ranges[2]);

			// Setup
			Rhi::RootSignatureBuilder rootSignatureBuilder;
			rootSignatureBuilder.initialize(static_cast<uint32_t>(GLM_COUNTOF(rootParameters)), rootParameters, 0, nullptr, Rhi::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			// Create the instance
			mRootSignature = rhi->createRootSignature(rootSignatureBuilder);
		}

		// Create sampler state and wrap it into a resource group instance
		Rhi::ISamplerState* linearSamplerResource = nullptr;
		Rhi::ISamplerState* pointSamplerResource = nullptr;
		{
			// Create the sampler resources
			Rhi::SamplerState samplerState = Rhi::ISamplerState::getDefaultSamplerState();
			samplerState.addressU = Rhi::TextureAddressMode::WRAP;
			samplerState.addressV = Rhi::TextureAddressMode::WRAP;
			linearSamplerResource = rhi->createSamplerState(samplerState);
			samplerState.filter = Rhi::FilterMode::MIN_MAG_MIP_POINT;
			pointSamplerResource = rhi->createSamplerState(samplerState);

			// Create the resource group
			Rhi::IResource* resources[2] = { linearSamplerResource, pointSamplerResource };
			mSamplerStateGroup = mRootSignature->createResourceGroup(1, static_cast<uint32_t>(GLM_COUNTOF(resources)), resources);
		}

		{ // Create the texture group
			Rhi::IResource* resources[2];

			{ // Create the 1D texture
				static constexpr uint32_t TEXTURE_WIDTH   = 256;
				static constexpr uint32_t TEXEL_ELEMENTS  = 1;
				static constexpr uint32_t NUMBER_OF_BYTES = TEXTURE_WIDTH;
				uint8_t data[NUMBER_OF_BYTES];

				// Fill the texture data with a color gradient
				for (uint32_t n = 0; n < NUMBER_OF_BYTES; n += TEXEL_ELEMENTS)
				{
					data[n] = static_cast<uint8_t>(n);
				}

				// Create the texture instance
				resources[0] = mTextureManager->createTexture1D(TEXTURE_WIDTH, Rhi::TextureFormat::R8, data, Rhi::TextureFlag::GENERATE_MIPMAPS | Rhi::TextureFlag::SHADER_RESOURCE);
			}

			{ // Create the 2D texture
				static constexpr uint32_t TEXTURE_WIDTH   = 64;
				static constexpr uint32_t TEXTURE_HEIGHT  = 64;
				static constexpr uint32_t TEXEL_ELEMENTS  = 4;
				static constexpr uint32_t NUMBER_OF_BYTES = TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXEL_ELEMENTS;
				uint8_t data[NUMBER_OF_BYTES];

				{ // Fill the texture data with a defective checkboard
					const uint32_t rowPitch   = TEXTURE_WIDTH * TEXEL_ELEMENTS;
					const uint32_t cellPitch  = rowPitch >> 3;		// The width of a cell in the checkboard texture
					const uint32_t cellHeight = TEXTURE_WIDTH >> 3;	// The height of a cell in the checkerboard texture
					for (uint32_t n = 0; n < NUMBER_OF_BYTES; n += TEXEL_ELEMENTS)
					{
						const uint32_t x = n % rowPitch;
						const uint32_t y = n / rowPitch;
						const uint32_t i = x / cellPitch;
						const uint32_t j = y / cellHeight;

						if (i % 2 == j % 2)
						{
							// Black
							data[n + 0] = 0;	// R
							data[n + 1] = 0;	// G
							data[n + 2] = 0;	// B
							data[n + 3] = 255;	// A
						}
						else
						{
							// Add some color fun instead of just boring white
							data[n + 0] = static_cast<uint8_t>(rand() % 255);	// R
							data[n + 1] = static_cast<uint8_t>(rand() % 255);	// G
							data[n + 2] = static_cast<uint8_t>(rand() % 255);	// B
							data[n + 3] = static_cast<uint8_t>(rand() % 255);	// A
						}
					}
				}

				// Create the texture instance
				resources[1] = mTextureManager->createTexture2D(TEXTURE_WIDTH, TEXTURE_HEIGHT, Rhi::TextureFormat::R8G8B8A8, data, Rhi::TextureFlag::GENERATE_MIPMAPS | Rhi::TextureFlag::SHADER_RESOURCE);
			}

			// Create the texture group
			Rhi::ISamplerState* samplerStates[2] = { linearSamplerResource, pointSamplerResource };
			mTextureGroup = mRootSignature->createResourceGroup(0, static_cast<uint32_t>(GLM_COUNTOF(resources)), resources, samplerStates);
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
		const Rhi::VertexAttributes vertexAttributes(static_cast<uint32_t>(GLM_COUNTOF(vertexAttributesLayout)), vertexAttributesLayout);

		{ // Create vertex array object (VAO)
			// Create the vertex buffer object (VBO)
			// -> Clip space vertex positions, left/bottom is (-1,-1) and right/top is (1,1)
			static constexpr float VERTEX_POSITION[] =
			{					// Vertex ID	Triangle on screen
				 0.0f, 1.0f,	// 0				0
				 1.0f, 0.0f,	// 1			   .   .
				-0.5f, 0.0f		// 2			  2.......1
			};
			Rhi::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION));

			// Create vertex array object (VAO)
			// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
			// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
			// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
			//    reference of the used vertex buffer objects (VBO). If the reference counter of a
			//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
			const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
			mVertexArray = mBufferManager->createVertexArray(vertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers);
		}

		{
			// Create the graphics program
			Rhi::IGraphicsProgramPtr graphicsProgram;
			{
				// Get the shader source code (outsourced to keep an overview)
				const char* vertexShaderSourceCode = nullptr;
				const char* fragmentShaderSourceCode = nullptr;
				#include "FirstTexture_GLSL_450.h"	// For Vulkan
				#include "FirstTexture_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
				#include "FirstTexture_GLSL_ES3.h"
				#include "FirstTexture_HLSL_D3D9.h"
				#include "FirstTexture_HLSL_D3D10_D3D11_D3D12.h"
				#include "FirstTexture_Null.h"

				// Create the graphics program
				Rhi::IShaderLanguage& shaderLanguage = rhi->getDefaultShaderLanguage();
				graphicsProgram = shaderLanguage.createGraphicsProgram(
					*mRootSignature,
					vertexAttributes,
					shaderLanguage.createVertexShaderFromSourceCode(vertexAttributes, vertexShaderSourceCode),
					shaderLanguage.createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
			}

			// Create the graphics pipeline state object (PSO)
			if (nullptr != graphicsProgram)
			{
				mGraphicsPipelineState = rhi->createGraphicsPipelineState(Rhi::GraphicsPipelineStateBuilder(mRootSignature, graphicsProgram, vertexAttributes, getMainRenderTarget()->getRenderPass()));
			}
		}

		// Since we're always submitting the same commands to the RHI, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
		fillCommandBuffer();
	}
}

void FirstTexture::onDeinitialization()
{
	// Release the used resources
	mVertexArray = nullptr;
	mGraphicsPipelineState = nullptr;
	mSamplerStateGroup = nullptr;
	mTextureGroup = nullptr;
	mRootSignature = nullptr;
	mCommandBuffer.clear();
	mTextureManager = nullptr;
	mBufferManager = nullptr;
}

void FirstTexture::onDraw()
{
	// Get and check the RHI instance
	Rhi::IRhiPtr rhi(getRhi());
	if (nullptr != rhi)
	{
		// Submit command buffer to the RHI implementation
		mCommandBuffer.submitToRhi(*rhi);
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void FirstTexture::fillCommandBuffer()
{
	// Sanity checks
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mRootSignature);
	assert(nullptr != mTextureGroup);
	assert(nullptr != mSamplerStateGroup);
	assert(nullptr != mGraphicsPipelineState);
	assert(nullptr != mVertexArray);

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Clear the graphics color buffer of the current render target with gray, do also clear the depth buffer
	Rhi::Command::ClearGraphics::create(mCommandBuffer, Rhi::ClearFlag::COLOR_DEPTH, Color4::GRAY);

	// Set the used graphics root signature
	Rhi::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// Set the used graphics pipeline state object (PSO)
	Rhi::Command::SetGraphicsPipelineState::create(mCommandBuffer, mGraphicsPipelineState);

	// Set graphics resource groups
	Rhi::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 0, mTextureGroup);
	Rhi::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 1, mSamplerStateGroup);

	// Input assembly (IA): Set the used vertex array
	Rhi::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArray);

	// Render the specified geometric primitive, based on an array of vertices
	Rhi::Command::DrawGraphics::create(mCommandBuffer, 3);
}
