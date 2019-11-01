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
#include "Renderer/Public/Resource/ShaderBlueprint/GraphicsShaderType.h"
#include "Renderer/Public/Resource/IResourceLoader.h"
#include "Renderer/Public/Core/GetInvalid.h"
#include "Renderer/Public/Core/File/MemoryFile.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class IRenderer;
	class MaterialBlueprintResource;
	template <class TYPE, class LOADER_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class ResourceManagerTemplate;
	namespace v1MaterialBlueprint
	{
		struct Texture;
		struct SamplerState;
	}
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t MaterialBlueprintResourceId;	///< POD material blueprint resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class MaterialBlueprintResourceLoader final : public IResourceLoader
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend ResourceManagerTemplate<MaterialBlueprintResource, MaterialBlueprintResourceLoader, MaterialBlueprintResourceId, 64>;	// Type definition of template class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("material_blueprint");


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IResourceLoader methods      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual ResourceLoaderTypeId getResourceLoaderTypeId() const override
		{
			return TYPE_ID;
		}

		virtual void initialize(const Asset& asset, bool reload, IResource& resource) override;

		[[nodiscard]] inline virtual bool hasDeserialization() const override
		{
			return true;
		}

		[[nodiscard]] virtual bool onDeserialization(IFile& file) override;

		[[nodiscard]] inline virtual bool hasProcessing() const override
		{
			return true;
		}

		virtual void onProcessing() override;
		[[nodiscard]] virtual bool onDispatch() override;
		[[nodiscard]] virtual bool isFullyLoaded() override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline MaterialBlueprintResourceLoader(IResourceManager& resourceManager, IRenderer& renderer) :
			IResourceLoader(resourceManager),
			mRenderer(renderer),
			mMaterialBlueprintResource(nullptr),
			mMaximumNumberOfRootParameters(0),
			mMaximumNumberOfDescriptorRanges(0),
			mVertexAttributesAssetId(getInvalid<AssetId>()),
			mGraphicsShaderBlueprintAssetId{getInvalid<AssetId>(), getInvalid<AssetId>(), getInvalid<AssetId>(), getInvalid<AssetId>(), getInvalid<AssetId>()},
			mComputeShaderBlueprintAssetId(getInvalid<AssetId>()),
			mMaximumNumberOfMaterialBlueprintSamplerStates(0),
			mMaterialBlueprintSamplerStates(nullptr),
			mMaximumNumberOfMaterialBlueprintTextures(0),
			mMaterialBlueprintTextures(nullptr)
		{
			// Nothing here
		}

		virtual ~MaterialBlueprintResourceLoader() override;
		explicit MaterialBlueprintResourceLoader(const MaterialBlueprintResourceLoader&) = delete;
		MaterialBlueprintResourceLoader& operator=(const MaterialBlueprintResourceLoader&) = delete;
		void createRhiResources();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRenderer&				   mRenderer;					///< Renderer instance, do not destroy the instance
		MaterialBlueprintResource* mMaterialBlueprintResource;	///< Destination resource

		// Temporary data
		MemoryFile mMemoryFile;

		// Temporary data: Root signature
		uint32_t						  mMaximumNumberOfRootParameters;
		std::vector<Rhi::RootParameter>   mRootParameters;
		uint32_t						  mMaximumNumberOfDescriptorRanges;
		std::vector<Rhi::DescriptorRange> mDescriptorRanges;
		Rhi::RootSignature				  mRootSignature;

		// Temporary data for graphics pipeline state: Vertex attributes and graphics shader blueprints
		AssetId mVertexAttributesAssetId;
		AssetId mGraphicsShaderBlueprintAssetId[NUMBER_OF_GRAPHICS_SHADER_TYPES];

		// Temporary data for compute pipeline state: Compute shader blueprint
		AssetId mComputeShaderBlueprintAssetId;

		// Temporary data: Sampler states
		uint32_t						   mMaximumNumberOfMaterialBlueprintSamplerStates;
		v1MaterialBlueprint::SamplerState* mMaterialBlueprintSamplerStates;

		// Temporary data: Textures
		uint32_t					  mMaximumNumberOfMaterialBlueprintTextures;
		v1MaterialBlueprint::Texture* mMaterialBlueprintTextures;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
