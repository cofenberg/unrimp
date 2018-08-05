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
#include "Advanced/InstancedCubes/CubeRendererDrawInstanced/CubeRendererDrawInstanced.h"
#include "Advanced/InstancedCubes/CubeRendererDrawInstanced/BatchDrawInstanced.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP

#include <math.h>
#include <float.h> // For FLT_MAX
#include <stdlib.h> // For rand()


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		// Vertex input layout
		static constexpr Renderer::VertexAttribute CubeRendererDrawInstancedVertexAttributesLayout[] =
		{
			{ // Attribute 0
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_3,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"Position",									// name[32] (char)
				"POSITION",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				0,											// alignedByteOffset (uint32_t)
				sizeof(float) * 8,							// strideInBytes (uint32_t)
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
				sizeof(float) * 3,							// alignedByteOffset (uint32_t)
				sizeof(float) * 8,							// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			},
			{ // Attribute 2
				// Data destination
				Renderer::VertexAttributeFormat::FLOAT_3,	// vertexAttributeFormat (Renderer::VertexAttributeFormat)
				"Normal",									// name[32] (char)
				"NORMAL",									// semanticName[32] (char)
				0,											// semanticIndex (uint32_t)
				// Data source
				0,											// inputSlot (uint32_t)
				sizeof(float) * 5,							// alignedByteOffset (uint32_t)
				sizeof(float) * 8,							// strideInBytes (uint32_t)
				0											// instancesPerElement (uint32_t)
			}
		};
		const Renderer::VertexAttributes CubeRendererDrawInstancedVertexAttributes(static_cast<uint32_t>(glm::countof(CubeRendererDrawInstancedVertexAttributesLayout)), CubeRendererDrawInstancedVertexAttributesLayout);


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
CubeRendererDrawInstanced::CubeRendererDrawInstanced(Renderer::IRenderer& renderer, Renderer::IRenderPass& renderPass, uint32_t numberOfTextures, uint32_t sceneRadius) :
	mRenderer(&renderer),
	mRenderPass(renderPass),
	mNumberOfTextures(numberOfTextures),
	mSceneRadius(sceneRadius),
	mMaximumNumberOfInstancesPerBatch(0),
	mNumberOfBatches(0),
	mBatches(nullptr)
{
	// Create the buffer and texture manager
	mBufferManager = mRenderer->createBufferManager();
	mTextureManager = mRenderer->createTextureManager();

	// Check number of textures (limit of this implementation and renderer limit)
	if (mNumberOfTextures > MAXIMUM_NUMBER_OF_TEXTURES)
	{
		mNumberOfTextures = MAXIMUM_NUMBER_OF_TEXTURES;
	}
	if (mNumberOfTextures > mRenderer->getCapabilities().maximumNumberOf2DTextureArraySlices)
	{
		mNumberOfTextures = mRenderer->getCapabilities().maximumNumberOf2DTextureArraySlices;
	}

	// Get the maximum number of instances per batch
	// -> In this application, this depends on the maximum texture buffer size
	// -> /2 -> One instance requires two texels
	mMaximumNumberOfInstancesPerBatch = mRenderer->getCapabilities().maximumTextureBufferSize / 2;

	{ // Create the root signature
		Renderer::DescriptorRangeBuilder ranges[6];
		ranges[0].initialize(Renderer::ResourceType::UNIFORM_BUFFER, 0, "UniformBlockStaticVs",		  Renderer::ShaderVisibility::VERTEX);
		ranges[1].initialize(Renderer::ResourceType::UNIFORM_BUFFER, 1, "UniformBlockDynamicVs",	  Renderer::ShaderVisibility::VERTEX);
		ranges[2].initialize(Renderer::ResourceType::TEXTURE_2D,	 0, "AlbedoMap",				  Renderer::ShaderVisibility::FRAGMENT);
		ranges[3].initialize(Renderer::ResourceType::UNIFORM_BUFFER, 0, "UniformBlockDynamicFs",	  Renderer::ShaderVisibility::FRAGMENT);
		ranges[4].initialize(Renderer::ResourceType::TEXTURE_BUFFER, 0, "PerInstanceTextureBufferVs", Renderer::ShaderVisibility::VERTEX);
		ranges[5].initializeSampler(0, Renderer::ShaderVisibility::FRAGMENT);

		Renderer::RootParameterBuilder rootParameters[3];
		rootParameters[0].initializeAsDescriptorTable(4, &ranges[0]);
		rootParameters[1].initializeAsDescriptorTable(1, &ranges[4]);
		rootParameters[2].initializeAsDescriptorTable(1, &ranges[5]);

		// Setup
		Renderer::RootSignatureBuilder rootSignature;
		rootSignature.initialize(static_cast<uint32_t>(glm::countof(rootParameters)), rootParameters, 0, nullptr, Renderer::RootSignatureFlags::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Create the instance
		mRootSignature = mRenderer->createRootSignature(rootSignature);
	}

	{ // Create the textures
		static constexpr uint32_t TEXTURE_WIDTH   = 128;
		static constexpr uint32_t TEXTURE_HEIGHT  = 128;
		static constexpr uint32_t NUMBER_OF_BYTES = TEXTURE_WIDTH * TEXTURE_HEIGHT * 4;

		// Allocate memory for the 2D texture array
		uint8_t* data = new uint8_t[NUMBER_OF_BYTES * mNumberOfTextures];

		{ // Fill the texture content
			// TODO(co) Be a little bit more creative while filling the texture data
			uint8_t* dataCurrent = data;
			const float colors[][MAXIMUM_NUMBER_OF_TEXTURES] =
			{
				{ 1.0f, 0.0f, 0.0f},
				{ 0.0f, 0.1f, 0.0f},
				{ 0.0f, 0.0f, 0.1f},
				{ 0.5f, 0.5f, 0.5f},
				{ 1.0f, 1.0f, 1.0f},
				{ 0.1f, 0.2f, 0.2f},
				{ 0.2f, 0.5f, 0.5f},
				{ 0.1f, 0.8f, 0.2f}
			};
			for (uint32_t j = 0; j < mNumberOfTextures; ++j)
			{
				// Random content
				for (uint32_t i = 0; i < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++i)
				{
					*dataCurrent = static_cast<uint8_t>((rand() % 255) * colors[j][0]);
					++dataCurrent;
					*dataCurrent = static_cast<uint8_t>((rand() % 255) * colors[j][1]);
					++dataCurrent;
					*dataCurrent = static_cast<uint8_t>((rand() % 255) * colors[j][2]);
					++dataCurrent;
					*dataCurrent = 255;
					++dataCurrent;
				}
			}
		}

		// Create the texture instance
		// -> By using 2D array textures together with OpenGL/Direct3D 11 instancing we get a handy implementation
		// -> This limits of course the cross platform support, fallback solutions might be a good idea in productive code
		// -> A fallback is not really required in our example situation because we're using draw instanced which already requires a more modern graphics card
		mTexture2DArray = mTextureManager->createTexture2DArray(TEXTURE_WIDTH, TEXTURE_HEIGHT, mNumberOfTextures, Renderer::TextureFormat::R8G8B8A8, data, Renderer::TextureFlag::GENERATE_MIPMAPS | Renderer::TextureFlag::SHADER_RESOURCE);

		// Free texture memory
		delete [] data;
	}

	// Create sampler state instance and wrap it into a resource group instance
	Renderer::IResource* samplerStateResource = mRenderer->createSamplerState(Renderer::ISamplerState::getDefaultSamplerState());
	mSamplerStateGroup = mRootSignature->createResourceGroup(2, 1, &samplerStateResource);

	{ // Create vertex array object (VAO)
		// Create the vertex buffer object (VBO)
		static constexpr float VERTEX_POSITION[] =
		{
			// Front face
			// Position					TexCoord		 Normal				// Vertex ID
			-0.5f, -0.5f,  0.5f,		0.0f, 0.0f,		 0.0f, 0.0f, 1.0f,	// 0
			 0.5f, -0.5f,  0.5f,		1.0f, 0.0f,		 0.0f, 0.0f, 1.0f,	// 1
			 0.5f,  0.5f,  0.5f,		1.0f, 1.0f,		 0.0f, 0.0f, 1.0f,	// 2
			-0.5f,  0.5f,  0.5f,		0.0f, 1.0f,		 0.0f, 0.0f, 1.0f,	// 3
			// Back face
			-0.5f, -0.5f, -0.5f,		1.0f, 0.0f,		 0.0f, 0.0f,-1.0f,	// 4
			-0.5f,  0.5f, -0.5f,		1.0f, 1.0f,		 0.0f, 0.0f,-1.0f,	// 5
			 0.5f,  0.5f, -0.5f,		0.0f, 1.0f,		 0.0f, 0.0f,-1.0f, 	// 6
			 0.5f, -0.5f, -0.5f,		0.0f, 0.0f,		 0.0f, 0.0f,-1.0f,	// 7
			// Top face
			-0.5f,  0.5f, -0.5f,		0.0f, 1.0f,		 0.0f, 1.0f, 0.0f,	// 8
			-0.5f,  0.5f,  0.5f,		0.0f, 0.0f,		 0.0f, 1.0f, 0.0f,	// 9
			 0.5f,  0.5f,  0.5f,		1.0f, 0.0f,		 0.0f, 1.0f, 0.0f,	// 10
			 0.5f,  0.5f, -0.5f,		1.0f, 1.0f,		 0.0f, 1.0f, 0.0f,	// 11
			// Bottom face
			-0.5f, -0.5f, -0.5f,		1.0f, 1.0f,		 0.0f,-1.0f, 0.0f,	// 12
			 0.5f, -0.5f, -0.5f,		0.0f, 1.0f,		 0.0f,-1.0f, 0.0f,	// 13
			 0.5f, -0.5f,  0.5f,		0.0f, 0.0f,		 0.0f,-1.0f, 0.0f,	// 14
			-0.5f, -0.5f,  0.5f,		1.0f, 0.0f,		 0.0f,-1.0f, 0.0f,	// 15
			// Right face
			 0.5f, -0.5f, -0.5f,		1.0f, 0.0f,		 1.0f, 0.0f, 0.0f,	// 16
			 0.5f,  0.5f, -0.5f,		1.0f, 1.0f,		 1.0f, 0.0f, 0.0f,	// 17
			 0.5f,  0.5f,  0.5f,		0.0f, 1.0f,		 1.0f, 0.0f, 0.0f,	// 18
			 0.5f, -0.5f,  0.5f,		0.0f, 0.0f,		 1.0f, 0.0f, 0.0f,	// 19
			// Left face
			-0.5f, -0.5f, -0.5f,		0.0f, 0.0f,		-1.0f, 0.0f, 0.0f,	// 20
			-0.5f, -0.5f,  0.5f,		1.0f, 0.0f,		-1.0f, 0.0f, 0.0f,	// 21
			-0.5f,  0.5f,  0.5f,		1.0f, 1.0f,		-1.0f, 0.0f, 0.0f,	// 22
			-0.5f,  0.5f, -0.5f,		0.0f, 1.0f,		-1.0f, 0.0f, 0.0f	// 23
		};
		Renderer::IVertexBufferPtr vertexBuffer(mBufferManager->createVertexBuffer(sizeof(VERTEX_POSITION), VERTEX_POSITION));

		// Create the index buffer object (IBO)
		static constexpr uint16_t INDICES[] =
		{
			// Front face	Triangle ID
			1,  0,  2,		// 0
			3,  2,  0,		// 1
			// Back face
			6,  5,  4,		// 2
			4,  7,  6,		// 3
			// Top face
			9,  8, 10,		// 4
			11, 10,  8,		// 5
			// Bottom face
			13, 12, 14,		// 6
			15, 14, 12,		// 7
			// Right face
			17, 16, 18,		// 8
			19, 18, 16,		// 9
			// Left face
			21, 20, 22,		// 10
			23, 22, 20		// 11
		};
		Renderer::IIndexBuffer* indexBuffer = mBufferManager->createIndexBuffer(sizeof(INDICES), INDICES);

		// Create vertex array object (VAO)
		// -> The vertex array object (VAO) keeps a reference to the used vertex buffer object (VBO)
		// -> This means that there's no need to keep an own vertex buffer object (VBO) reference
		// -> When the vertex array object (VAO) is destroyed, it automatically decreases the
		//    reference of the used vertex buffer objects (VBO). If the reference counter of a
		//    vertex buffer object (VBO) reaches zero, it's automatically destroyed.
		const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer };
		mVertexArray = mBufferManager->createVertexArray(detail::CubeRendererDrawInstancedVertexAttributes, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, indexBuffer);
	}

	// Uniform buffer object (UBO, "constant buffer" in Direct3D terminology) supported?
	// -> If they are there, we really want to use them (performance and ease of use)
	if (mRenderer->getCapabilities().maximumUniformBufferSize > 0)
	{
		{ // Create and set constant graphics program uniform buffer at once
			// TODO(co) Ugly fixed hacked in model-view-projection matrix
			// TODO(co) OpenGL matrix, Direct3D has minor differences within the projection matrix we have to compensate
			static constexpr float MVP[] =
			{
				 1.2803299f,	-0.97915620f,	-0.58038759f,	-0.57922798f,
				 0.0f,			 1.9776078f,	-0.57472473f,	-0.573576453f,
				-1.2803299f,	-0.97915620f,	-0.58038759f,	-0.57922798f,
				 0.0f,			 0.0f,			 9.8198195f,	 10.0f
			};
			mUniformBufferStaticVs = mBufferManager->createUniformBuffer(sizeof(MVP), MVP);
		}

		// Create dynamic uniform buffers
		mUniformBufferDynamicVs = mBufferManager->createUniformBuffer(sizeof(float) * 2, nullptr, Renderer::BufferUsage::DYNAMIC_DRAW);
		mUniformBufferDynamicFs = mBufferManager->createUniformBuffer(sizeof(float) * 3, nullptr, Renderer::BufferUsage::DYNAMIC_DRAW);
	}

	{ // Create resource group
		Renderer::IResource* resources[4] = { mUniformBufferStaticVs, mUniformBufferDynamicVs, mTexture2DArray, mUniformBufferDynamicFs };
		Renderer::ISamplerState* samplerStates[4] = { nullptr, nullptr, static_cast<Renderer::ISamplerState*>(samplerStateResource), nullptr };
		mResourceGroup = mRootSignature->createResourceGroup(0, static_cast<uint32_t>(glm::countof(resources)), resources, samplerStates);
	}

	// Create the graphics program: Decide which shader language should be used (for example "GLSL" or "HLSL")
	Renderer::IShaderLanguagePtr shaderLanguage(mRenderer->getShaderLanguage());
	if (nullptr != shaderLanguage)
	{
		// Get the shader source code (outsourced to keep an overview)
		const char* vertexShaderSourceCode = nullptr;
		const char* fragmentShaderSourceCode = nullptr;
		#include "CubeRendererDrawInstanced_GLSL_450.h"	// For Vulkan
		#include "CubeRendererDrawInstanced_GLSL_410.h"	// macOS 10.11 only supports OpenGL 4.1 hence it's our OpenGL minimum
		#include "CubeRendererDrawInstanced_HLSL_D3D10_D3D11_D3D12.h"
		#include "CubeRendererDrawInstanced_GLSL_ES3.h"
		#include "CubeRendererDrawInstanced_Null.h"

		// Create the graphics program
		mGraphicsProgram = shaderLanguage->createGraphicsProgram(
			*mRootSignature,
			detail::CubeRendererDrawInstancedVertexAttributes,
			shaderLanguage->createVertexShaderFromSourceCode(detail::CubeRendererDrawInstancedVertexAttributes, vertexShaderSourceCode),
			shaderLanguage->createFragmentShaderFromSourceCode(fragmentShaderSourceCode));
	}
}

CubeRendererDrawInstanced::~CubeRendererDrawInstanced()
{
	// The renderer resource pointers are released automatically

	// Destroy the batches, if needed
	delete [] mBatches;
}


//[-------------------------------------------------------]
//[ Public virtual ICubeRenderer methods                  ]
//[-------------------------------------------------------]
void CubeRendererDrawInstanced::setNumberOfCubes(uint32_t numberOfCubes)
{
	// Destroy previous batches, in case there are any
	if (nullptr != mBatches)
	{
		delete [] mBatches;
		mNumberOfBatches = 0;
		mBatches = nullptr;
	}

	// A third of the cubes should be rendered using alpha blending
	const uint32_t numberOfTransparentCubes = numberOfCubes / 3;
	const uint32_t numberOfSolidCubes       = numberOfCubes - numberOfTransparentCubes;

	// There's a limitation how many instances can be created per draw call
	// -> If required, create multiple batches
	const uint32_t numberOfSolidBatches       = static_cast<uint32_t>(ceil(static_cast<float>(numberOfSolidCubes)       / mMaximumNumberOfInstancesPerBatch));
	const uint32_t numberOfTransparentBatches = static_cast<uint32_t>(ceil(static_cast<float>(numberOfTransparentCubes) / mMaximumNumberOfInstancesPerBatch));

	// Create a batch instances
	mNumberOfBatches = numberOfSolidBatches + numberOfTransparentBatches;
	mBatches = new BatchDrawInstanced[mNumberOfBatches];

	// Initialize the solid batches
	BatchDrawInstanced* batch     = mBatches;
	BatchDrawInstanced* lastBatch = mBatches + numberOfSolidBatches;
	for (int remaningNumberOfCubes = static_cast<int>(numberOfSolidCubes); batch < lastBatch; ++batch, remaningNumberOfCubes -= mMaximumNumberOfInstancesPerBatch)
	{
		const uint32_t currentNumberOfCubes = (remaningNumberOfCubes > static_cast<int>(mMaximumNumberOfInstancesPerBatch)) ? mMaximumNumberOfInstancesPerBatch : remaningNumberOfCubes;
		batch->initialize(*mBufferManager, *mRootSignature, detail::CubeRendererDrawInstancedVertexAttributes, *mGraphicsProgram, mRenderPass, currentNumberOfCubes, false, mNumberOfTextures, mSceneRadius);
	}

	// Initialize the transparent batches
	// -> TODO(co) For correct alpha blending, the transparent instances should be sorted from back to front.
	lastBatch = batch + numberOfTransparentBatches;
	for (int remaningNumberOfCubes = static_cast<int>(numberOfTransparentCubes); batch < lastBatch; ++batch, remaningNumberOfCubes -= mMaximumNumberOfInstancesPerBatch)
	{
		const uint32_t currentNumberOfCubes = (remaningNumberOfCubes > static_cast<int>(mMaximumNumberOfInstancesPerBatch)) ? mMaximumNumberOfInstancesPerBatch : remaningNumberOfCubes;
		batch->initialize(*mBufferManager, *mRootSignature, detail::CubeRendererDrawInstancedVertexAttributes, *mGraphicsProgram, mRenderPass, currentNumberOfCubes, true, mNumberOfTextures, mSceneRadius);
	}

	// Since we're always submitting the same commands to the renderer, we can fill the command buffer once during initialization and then reuse it multiple times during runtime
	mCommandBuffer.clear();
	fillReusableCommandBuffer();
}

void CubeRendererDrawInstanced::fillCommandBuffer(float globalTimer, float globalScale, float lightPositionX, float lightPositionY, float lightPositionZ, Renderer::CommandBuffer& commandBuffer)
{
	// Sanity checks
	assert(nullptr != mGraphicsProgram);

	{ // Update graphics program uniform data
		// Some counting timer, we don't want to touch the buffers on the GPU
		const float timerAndGlobalScale[] = { globalTimer, globalScale };

		// Animate the point light world space position
		const float lightPosition[] = { lightPositionX, lightPositionY, lightPositionZ };

		// Use uniform buffer?
		if (nullptr != mUniformBufferDynamicVs)
		{
			// Copy data into the uniform buffer
			Renderer::MappedSubresource mappedSubresource;
			if (mRenderer->map(*mUniformBufferDynamicVs, 0, Renderer::MapType::WRITE_DISCARD, 0, mappedSubresource))
			{
				memcpy(mappedSubresource.data, timerAndGlobalScale, sizeof(timerAndGlobalScale));
				mRenderer->unmap(*mUniformBufferDynamicVs, 0);
			}
			if (nullptr != mUniformBufferDynamicFs && mRenderer->map(*mUniformBufferDynamicFs, 0, Renderer::MapType::WRITE_DISCARD, 0, mappedSubresource))
			{
				memcpy(mappedSubresource.data, lightPosition, sizeof(lightPosition));
				mRenderer->unmap(*mUniformBufferDynamicFs, 0);
			}
		}
		else
		{
			// Set individual graphics program uniforms
			// -> Using uniform buffers (aka constant buffers in Direct3D) would be more efficient, but Direct3D 9 doesn't support it (neither does e.g. OpenGL ES 3.0)
			// -> To keep it simple in here, I just use a less efficient string to identity the uniform (does not really hurt in here)
			// TODO(co) Update
			// mGraphicsProgram->setUniform2fv(mGraphicsProgram->getUniformHandle("TimerAndGlobalScale"), timerAndGlobalScale);
			// mGraphicsProgram->setUniform3fv(mGraphicsProgram->getUniformHandle("LightPosition"), lightPosition);
		}
	}

	// Set constant graphics program uniform
	if (nullptr == mUniformBufferStaticVs)
	{
		// TODO(co) Ugly fixed hacked in model-view-projection matrix
		// TODO(co) OpenGL matrix, Direct3D has minor differences within the projection matrix we have to compensate
// 		static constexpr float MVP[] =
// 		{
// 				1.2803299f,	-0.97915620f,	-0.58038759f,	-0.57922798f,
// 				0.0f,			 1.9776078f,	-0.57472473f,	-0.573576453f,
// 			-1.2803299f,	-0.97915620f,	-0.58038759f,	-0.57922798f,
// 				0.0f,			 0.0f,			 9.8198195f,	 10.0f
// 		};

		// There's no uniform buffer: We have to set individual uniforms
		// TODO(co) Update
		// mGraphicsProgram->setUniform4fv(mGraphicsProgram->getUniformHandle("MVP"), MVP);
	}

	// Execute pre-recorded command buffer
	Renderer::Command::ExecuteCommandBuffer::create(commandBuffer, &mCommandBuffer);
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void CubeRendererDrawInstanced::fillReusableCommandBuffer()
{
	// Sanity checks
	assert(mCommandBuffer.isEmpty());
	assert(nullptr != mRootSignature);
	assert(nullptr != mTexture2DArray);
	assert(0 == mRenderer->getCapabilities().maximumUniformBufferSize || nullptr != mUniformBufferStaticVs);
	assert(0 == mRenderer->getCapabilities().maximumUniformBufferSize || nullptr != mUniformBufferDynamicVs);
	assert(0 == mRenderer->getCapabilities().maximumUniformBufferSize || nullptr != mUniformBufferDynamicFs);
	assert(nullptr != mResourceGroup);
	assert(nullptr != mSamplerStateGroup);
	assert(nullptr != mVertexArray);

	// Scoped debug event
	COMMAND_SCOPED_DEBUG_EVENT_FUNCTION(mCommandBuffer)

	// Set the used graphics root signature
	Renderer::Command::SetGraphicsRootSignature::create(mCommandBuffer, mRootSignature);

	// Set resource groups
	Renderer::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 0, mResourceGroup);
	// Graphics root descriptor table 1 is set inside "BatchDrawInstanced::draw()"
	Renderer::Command::SetGraphicsResourceGroup::create(mCommandBuffer, 2, mSamplerStateGroup);

	// Input assembly (IA): Set the used vertex array
	Renderer::Command::SetGraphicsVertexArray::create(mCommandBuffer, mVertexArray);

	// Draw the batches
	if (nullptr != mBatches)
	{
		// Loop though all batches
		BatchDrawInstanced *batch     = mBatches;
		BatchDrawInstanced *lastBatch = mBatches + mNumberOfBatches;
		for (; batch < lastBatch; ++batch)
		{
			// Draw this batch
			batch->fillCommandBuffer(mCommandBuffer);
		}
	}
}
