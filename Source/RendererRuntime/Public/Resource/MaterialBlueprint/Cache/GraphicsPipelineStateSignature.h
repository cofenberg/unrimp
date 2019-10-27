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
#include "RendererRuntime/Public/Resource/ShaderBlueprint/Cache/ShaderProperties.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class ShaderBlueprintResource;
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
	typedef uint32_t MaterialBlueprintResourceId;		///< POD material blueprint resource identifier
	typedef uint32_t GraphicsPipelineStateSignatureId;	///< Graphics pipeline state signature identifier, result of hashing the referenced shaders as well as other pipeline state properties
	typedef uint32_t ShaderCombinationId;				///< Shader combination identifier, result of hashing the shader combination generating shader blueprint resource, shader properties and dynamic shader pieces


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Graphics pipeline state signature
	*
	*  @see
	*    - See "RendererRuntime::GraphicsPipelineStateCacheManager" for additional information
	*/
	class GraphicsPipelineStateSignature final
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] static ShaderCombinationId generateShaderCombinationId(const ShaderBlueprintResource& shaderBlueprintResource, const ShaderProperties& shaderProperties);


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		inline GraphicsPipelineStateSignature() :
			mMaterialBlueprintResourceId(getInvalid<MaterialBlueprintResourceId>()),
			mSerializedGraphicsPipelineStateHash(getInvalid<uint32_t>()),
			mGraphicsPipelineStateSignatureId(getInvalid<GraphicsPipelineStateSignatureId>()),
			mShaderCombinationId{getInvalid<ShaderCombinationId>(), getInvalid<ShaderCombinationId>(), getInvalid<ShaderCombinationId>(), getInvalid<ShaderCombinationId>(), getInvalid<ShaderCombinationId>()}
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] materialBlueprintResource
		*    Material blueprint resource to use
		*  @param[in] serializedGraphicsPipelineStateHash
		*    FNV1a hash of "Rhi::SerializedGraphicsPipelineState"
		*  @param[in] shaderProperties
		*    Shader properties to use, you should ensure that this shader properties are already optimized by using e.g. "RendererRuntime::MaterialBlueprintResource::optimizeShaderProperties()"
		*/
		inline GraphicsPipelineStateSignature(const MaterialBlueprintResource& materialBlueprintResource, uint32_t serializedGraphicsPipelineStateHash, const ShaderProperties& shaderProperties)
		{
			set(materialBlueprintResource, serializedGraphicsPipelineStateHash, shaderProperties);
		}

		/**
		*  @brief
		*    Copy constructor
		*
		*  @param[in] graphicsPipelineStateSignature
		*    Graphics pipeline state signature to copy from
		*/
		explicit GraphicsPipelineStateSignature(const GraphicsPipelineStateSignature& graphicsPipelineStateSignature);

		/**
		*  @brief
		*    Destructor
		*/
		inline ~GraphicsPipelineStateSignature()
		{
			// Nothing here
		}

		/**
		*  @brief
		*    Copy operator
		*/
		GraphicsPipelineStateSignature& operator=(const GraphicsPipelineStateSignature& graphicsPipelineStateSignature);

		/**
		*  @brief
		*    Setter
		*
		*  @param[in] materialBlueprintResource
		*    Material blueprint resource to use
		*  @param[in] serializedGraphicsPipelineStateHash
		*    FNV1a hash of "Rhi::SerializedGraphicsPipelineState"
		*  @param[in] shaderProperties
		*    Shader properties to use, you should ensure that this shader properties are already optimized by using e.g. "RendererRuntime::MaterialBlueprintResource::optimizeShaderProperties()"
		*/
		void set(const MaterialBlueprintResource& materialBlueprintResource, uint32_t serializedGraphicsPipelineStateHash, const ShaderProperties& shaderProperties);

		//[-------------------------------------------------------]
		//[ Getter for input data                                 ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline MaterialBlueprintResourceId getMaterialBlueprintResourceId() const
		{
			return mMaterialBlueprintResourceId;
		}

		[[nodiscard]] inline uint32_t getSerializedGraphicsPipelineStateHash() const
		{
			return mSerializedGraphicsPipelineStateHash;
		}

		[[nodiscard]] inline const ShaderProperties& getShaderProperties() const
		{
			return mShaderProperties;
		}

		//[-------------------------------------------------------]
		//[ Getter for derived data                               ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline GraphicsPipelineStateSignatureId getGraphicsPipelineStateSignatureId() const
		{
			return mGraphicsPipelineStateSignatureId;
		}

		[[nodiscard]] inline ShaderCombinationId getShaderCombinationId(GraphicsShaderType graphicsShaderType) const
		{
			return mShaderCombinationId[static_cast<uint8_t>(graphicsShaderType)];
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Input data
		MaterialBlueprintResourceId	mMaterialBlueprintResourceId;
		uint32_t					mSerializedGraphicsPipelineStateHash;
		ShaderProperties			mShaderProperties;
		// Derived data
		GraphicsPipelineStateSignatureId mGraphicsPipelineStateSignatureId;
		ShaderCombinationId				 mShaderCombinationId[NUMBER_OF_GRAPHICS_SHADER_TYPES];


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
