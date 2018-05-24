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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "RendererRuntime/Core/GetUninitialized.h"
#include "RendererRuntime/Resource/ShaderBlueprint/Cache/ShaderProperties.h"

#include <Renderer/Renderer.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class MaterialBlueprintResource;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t MaterialBlueprintResourceId;	///< POD material blueprint resource identifier
	typedef uint32_t PipelineStateSignatureId;		///< Pipeline state signature identifier, result of hashing the referenced shaders as well as other pipeline state properties
	typedef uint32_t ShaderCombinationId;			///< Shader combination identifier, result of hashing the shader combination generating shader blueprint resource, shader properties and dynamic shader pieces


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Pipeline state signature
	*
	*  @see
	*    - See "RendererRuntime::PipelineStateCacheManager" for additional information
	*/
	class PipelineStateSignature final
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		inline PipelineStateSignature() :
			mMaterialBlueprintResourceId(getUninitialized<MaterialBlueprintResourceId>()),
			mSerializedPipelineStateHash(getUninitialized<uint32_t>()),
			mPipelineStateSignatureId(getUninitialized<PipelineStateSignatureId>()),
			mShaderCombinationId{getUninitialized<ShaderCombinationId>(), getUninitialized<ShaderCombinationId>(), getUninitialized<ShaderCombinationId>(), getUninitialized<ShaderCombinationId>(), getUninitialized<ShaderCombinationId>()}
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] materialBlueprintResource
		*    Material blueprint resource to use
		*  @param[in] serializedPipelineStateHash
		*    FNV1a hash of "Renderer::SerializedPipelineState"
		*  @param[in] shaderProperties
		*    Shader properties to use, you should ensure that this shader properties are already optimized by using e.g. "RendererRuntime::MaterialBlueprintResource::optimizeShaderProperties()"
		*/
		inline PipelineStateSignature(const MaterialBlueprintResource& materialBlueprintResource, uint32_t serializedPipelineStateHash, const ShaderProperties& shaderProperties)
		{
			set(materialBlueprintResource, serializedPipelineStateHash, shaderProperties);
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] pipelineStateSignature
		*    Pipeline state signature to copy from
		*/
		explicit PipelineStateSignature(const PipelineStateSignature& pipelineStateSignature);

		/**
		*  @brief
		*    Destructor
		*/
		inline ~PipelineStateSignature()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy operator
		*/
		PipelineStateSignature& operator=(const PipelineStateSignature& pipelineStateSignature);

		/**
		*  @brief
		*    Setter
		*
		*  @param[in] materialBlueprintResource
		*    Material blueprint resource to use
		*  @param[in] serializedPipelineStateHash
		*    FNV1a hash of "Renderer::SerializedPipelineState"
		*  @param[in] shaderProperties
		*    Shader properties to use, you should ensure that this shader properties are already optimized by using e.g. "RendererRuntime::MaterialBlueprintResource::optimizeShaderProperties()"
		*/
		void set(const MaterialBlueprintResource& materialBlueprintResource, uint32_t serializedPipelineStateHash, const ShaderProperties& shaderProperties);

		//[-------------------------------------------------------]
		//[ Getter for input data                                 ]
		//[-------------------------------------------------------]
		inline MaterialBlueprintResourceId getMaterialBlueprintResourceId() const
		{
			return mMaterialBlueprintResourceId;
		}

		inline uint32_t getSerializedPipelineStateHash() const
		{
			return mSerializedPipelineStateHash;
		}

		inline const ShaderProperties& getShaderProperties() const
		{
			return mShaderProperties;
		}

		//[-------------------------------------------------------]
		//[ Getter for derived data                               ]
		//[-------------------------------------------------------]
		inline PipelineStateSignatureId getPipelineStateSignatureId() const
		{
			return mPipelineStateSignatureId;
		}

		inline ShaderCombinationId getShaderCombinationId(ShaderType shaderType) const
		{
			return mShaderCombinationId[static_cast<uint8_t>(shaderType)];
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Input data
		MaterialBlueprintResourceId	mMaterialBlueprintResourceId;
		uint32_t					mSerializedPipelineStateHash;
		ShaderProperties			mShaderProperties;
		// Derived data
		PipelineStateSignatureId mPipelineStateSignatureId;
		ShaderCombinationId		 mShaderCombinationId[NUMBER_OF_SHADER_TYPES];


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
