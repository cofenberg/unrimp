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
#include <Rhi/Public/Rhi.h>


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Batch of instanced cubes using instanced arrays
*/
class BatchInstancedArrays final
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Default constructor
	*/
	inline BatchInstancedArrays() :
		mNumberOfCubeInstances(0)
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Destructor
	*/
	inline ~BatchInstancedArrays()
	{
		// The RHI resource pointers are released automatically
	}

	/**
	*  @brief
	*    Initialize the batch
	*
	*  @param[in] bufferManager
	*    Buffer manager to use
	*  @param[in] rootSignature
	*    Root signature
	*  @param[in] vertexAttributes
	*    Vertex attributes ("vertex declaration" in Direct3D 9 terminology, "input layout" in Direct3D 10 & 11 terminology)
	*  @param[in] vertexBuffer
	*    Vertex buffer object (VBO) containing the mesh data to use
	*  @param[in] indexBuffer
	*    Index buffer object (IBO) to use
	*  @param[in] graphicsProgram
	*    Graphics program used for rendering
	*  @param[in] renderPass
	*    Render pass used for rendering
	*  @param[in] numberOfCubeInstances
	*    Number of cube instances
	*  @param[in] alphaBlending
	*    Does this batch use alpha blending?
	*  @param[in] numberOfTextures
	*    Number of textures
	*  @param[in] sceneRadius
	*    Scene radius
	*/
	void initialize(Rhi::IBufferManager& bufferManager, Rhi::IRootSignature& rootSignature, const Rhi::VertexAttributes& vertexAttributes, Rhi::IVertexBuffer& vertexBuffer, Rhi::IIndexBuffer& indexBuffer, Rhi::IGraphicsProgram& graphicsProgram, Rhi::IRenderPass& renderPass, uint32_t numberOfCubeInstances, bool alphaBlending, uint32_t numberOfTextures, uint32_t sceneRadius);

	/**
	*  @brief
	*    Fill the batch into the given commando buffer
	*
	*  @param[out] commandBuffer
	*    RHI command buffer to fill
	*/
	void fillCommandBuffer(Rhi::CommandBuffer& commandBuffer) const;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit BatchInstancedArrays(const BatchInstancedArrays& source) = delete;
	BatchInstancedArrays& operator =(const BatchInstancedArrays& source) = delete;


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Rhi::IRhiPtr					mRhi;					///< Owner RHI instance, can be a null pointer
	uint32_t						mNumberOfCubeInstances;	///< Number of cube instances
	Rhi::IVertexArrayPtr			mVertexArray;			///< Vertex array object (VAO), can be a null pointer
	Rhi::IGraphicsPipelineStatePtr	mGraphicsPipelineState;	///< Graphics pipeline state object (PSO), can be a null pointer


};
