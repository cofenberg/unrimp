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
#include "Examples/Private/Framework/PlatformTypes.h"
#include "Examples/Private/Framework/ExampleBase.h"

#include <RendererRuntime/Public/Core/Time/Stopwatch.h>
#include <RendererRuntime/Public/Resource/IResourceListener.h>


//[-------------------------------------------------------]
//[ Global definitions                                    ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	typedef uint32_t MeshResourceId;	///< POD mesh resource identifier
	typedef uint32_t TextureResourceId;	///< POD texture resource identifier
}


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    First mesh example
*
*  @remarks
*    Demonstrates:
*    - Index buffer object (IBO)
*    - Vertex buffer object (VBO)
*    - Vertex array object (VAO)
*    - Texture buffer object (TBO)
*    - Uniform buffer object (UBO)
*    - Sampler state object
*    - Vertex shader (VS) and fragment shader (FS)
*    - Root signature
*    - Graphics pipeline state object (PSO)
*    - Blinn-Phong shading
*    - Albedo, normal, roughness and emissive mapping
*    - Optimization: Cache data to not bother the RHI to much
*    - Compact vertex format (32 bit texture coordinate, QTangent, 56 bytes vs. 28 bytes per vertex)
*/
class FirstMesh final : public ExampleBase, public RendererRuntime::IResourceListener
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*/
	FirstMesh();

	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~FirstMesh() override
	{
		// The resources are released within "onDeinitialization()"
		// Nothing here
	}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization() override;
	virtual void onDeinitialization() override;
	virtual void onUpdate() override;
	virtual void onDraw() override;


//[-------------------------------------------------------]
//[ Protected virtual RendererRuntime::IResourceListener methods ]
//[-------------------------------------------------------]
protected:
	inline virtual void onLoadingStateChange(const RendererRuntime::IResource&) override
	{
		// Forget about the resource group so it's rebuild
		mResourceGroup = nullptr;
		mCommandBuffer.clear();
		fillCommandBuffer();
	}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit FirstMesh(const FirstMesh&) = delete;
	FirstMesh& operator=(const FirstMesh&) = delete;
	void fillCommandBuffer();


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Rhi::CommandBuffer				   mCommandBuffer;				///< Command buffer
	Rhi::IRootSignaturePtr			   mRootSignature;				///< Root signature, can be a null pointer
	Rhi::IUniformBufferPtr			   mUniformBuffer;				///< Uniform buffer object (UBO), can be a null pointer
	Rhi::IGraphicsPipelineStatePtr	   mGraphicsPipelineState;		///< Graphics pipeline state object (PSO), can be a null pointer
	Rhi::IGraphicsProgramPtr		   mGraphicsProgram;			///< Graphics program, can be a null pointer
	RendererRuntime::MeshResourceId	   mMeshResourceId;				///< Mesh resource ID, can be set to invalid value
	RendererRuntime::TextureResourceId m_argb_nxaTextureResourceId;
	RendererRuntime::TextureResourceId m_hr_rg_mb_nyaTextureResourceId;
	RendererRuntime::TextureResourceId mEmissiveTextureResourceId;
	Rhi::IResourceGroupPtr			   mResourceGroup;				///< Resource group, can be a null pointer
	Rhi::ISamplerStatePtr			   mSamplerStatePtr;			///< Sampler state, can be a null pointer
	Rhi::IResourceGroupPtr			   mSamplerStateGroup;			///< Sampler state resource group, can be a null pointer
	// Optimization: Cache data to not bother the RHI implementation to much
	handle mObjectSpaceToClipSpaceMatrixUniformHandle;	///< Object space to clip space matrix uniform handle, can be "NULL_HANDLE"
	handle mObjectSpaceToViewSpaceMatrixUniformHandle;	///< Object space to view space matrix uniform handle, can be "NULL_HANDLE"
	// For timing
	RendererRuntime::Stopwatch mStopwatch;		///< Stopwatch instance
	float					   mGlobalTimer;	///< Global timer


};
