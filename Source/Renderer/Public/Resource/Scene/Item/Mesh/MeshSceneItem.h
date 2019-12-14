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
#include "Renderer/Public/Resource/Scene/Item/ISceneItem.h"
#include "Renderer/Public/Resource/IResourceListener.h"
#include "Renderer/Public/RenderQueue/RenderableManager.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;			///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"
	typedef uint32_t MeshResourceId;	///< POD mesh resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class MeshSceneItem : public ISceneItem, public IResourceListener
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SceneFactory;	// Needs to be able to create scene item instances


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("MeshSceneItem");


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline MeshResourceId getMeshResourceId() const
		{
			return mMeshResourceId;
		}

		RENDERER_API_EXPORT void setMeshResourceId(MeshResourceId meshResourceId);
		RENDERER_API_EXPORT void setMeshResourceIdByAssetId(AssetId meshAssetId);

		[[nodiscard]] inline uint32_t getNumberOfSubMeshes() const
		{
			// The renderables contain all LODs, each LOD has the same number of renderables
			return static_cast<uint32_t>(mRenderableManager.getRenderables().size() / mRenderableManager.getNumberOfLods());
		}

		[[nodiscard]] inline uint8_t getNumberOfLods() const
		{
			return mRenderableManager.getNumberOfLods();
		}

		[[nodiscard]] inline MaterialResourceId getMaterialResourceIdOfSubMeshLod(uint32_t subMeshIndex, uint8_t lodIndex) const
		{
			// The renderables contain all LODs, each LOD has the same number of renderables
			RHI_ASSERT(getContext(), subMeshIndex < getNumberOfSubMeshes(), "Invalid sub mesh index")
			RHI_ASSERT(getContext(), lodIndex < mRenderableManager.getNumberOfLods(), "Invalid LOD index")
			return mRenderableManager.getRenderables()[subMeshIndex + lodIndex * getNumberOfSubMeshes()].getMaterialResourceId();
		}

		RENDERER_API_EXPORT void setMaterialResourceIdOfSubMeshLod(uint32_t subMeshIndex, uint8_t lodIndex, MaterialResourceId materialResourceId);
		RENDERER_API_EXPORT void setMaterialResourceIdOfAllSubMeshesAndLods(MaterialResourceId materialResourceId);


	//[-------------------------------------------------------]
	//[ Public Renderer::ISceneItem methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual SceneItemTypeId getSceneItemTypeId() const override
		{
			return TYPE_ID;
		}

		virtual void deserialize(uint32_t numberOfBytes, const uint8_t* data) override;
		virtual void onAttachedToSceneNode(SceneNode& sceneNode) override;

		inline virtual void onDetachedFromSceneNode(SceneNode& sceneNode) override
		{
			mRenderableManager.setTransform(nullptr);

			// Call the base implementation
			ISceneItem::onDetachedFromSceneNode(sceneNode);
		}

		inline virtual void setVisible(bool visible) override
		{
			mRenderableManager.setVisible(visible);
		}

		[[nodiscard]] inline virtual const RenderableManager* getRenderableManager() const override
		{
			return &mRenderableManager;
		}


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline explicit MeshSceneItem(SceneResource& sceneResource) :
			ISceneItem(sceneResource),
			mMeshResourceId(getInvalid<MeshResourceId>())
		{
			// Nothing here
		}

		inline virtual ~MeshSceneItem() override
		{
			// Nothing here
		}

		explicit MeshSceneItem(const MeshSceneItem&) = delete;
		MeshSceneItem& operator=(const MeshSceneItem&) = delete;


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::IResourceListener methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void onLoadingStateChange(const IResource& resource) override;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::vector<AssetId> SubMeshMaterialAssetIds;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		MeshResourceId			mMeshResourceId;			///< Mesh resource ID, can be set to invalid value
		SubMeshMaterialAssetIds	mSubMeshMaterialAssetIds;	///< Sub-mesh material asset IDs received during deserialization
		RenderableManager		mRenderableManager;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
