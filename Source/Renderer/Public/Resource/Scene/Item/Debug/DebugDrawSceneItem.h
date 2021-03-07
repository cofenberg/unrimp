/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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
#include "Renderer/Public/RenderQueue/RenderableManager.h"
#include "Renderer/Public/Resource/IResourceListener.h"
#include "Renderer/Public/Resource/Scene/Item/ISceneItem.h"
#include "Renderer/Public/Resource/Material/MaterialProperties.h"

#include <glm/fwd.hpp>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;				///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"
	typedef std::vector<AssetId> AssetIds;
	typedef uint32_t MaterialTechniqueId;	///< Material technique identifier, result of hashing the material technique name via "Renderer::StringId"
	typedef uint32_t MaterialResourceId;	///< POD material resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Debug draw scene item
	*/
	class DebugDrawSceneItem final : public ISceneItem, public IResourceListener
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class SceneFactory;	// Needs to be able to create scene item instances


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("DebugDrawSceneItem");
		enum RenderableIndex : uint8_t
		{
			POINT_LIST_DEPTH_DISABLED,
			POINT_LIST_DEPTH_ENABLED,
			LINE_LIST_DEPTH_DISABLED,
			LINE_LIST_DEPTH_ENABLED,
			GLYPH_LIST,
			NUMBER_OF_INDICES
		};


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the asset IDs of automatically generated dynamic default texture assets
		*
		*  @param[out] assetIds
		*    Receives the asset IDs of automatically generated dynamic default texture assets, the list is not cleared before new entries are added
		*
		*  @remarks
		*    The debug draw scene item automatically generates some dynamic default texture assets one can reference e.g. inside material blueprint resources:
		*    - "Unrimp/Texture/DynamicByCode/DebugDrawGlyphMap2D"
		*/
		static void getDefaultTextureAssetIds(AssetIds& assetIds);


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Clear
		*/
		RENDERER_API_EXPORT void clear();

		/**
		*  @brief
		*    Flush
		*
		*  @note
		*    - Call this once per frame
		*/
		RENDERER_API_EXPORT void flush();

		//[-------------------------------------------------------]
		//[ Draw methods                                          ]
		//[-------------------------------------------------------]
		RENDERER_API_EXPORT void drawPoint(const glm::vec3& worldSpacePosition, const float sRgbColor[3], float size = 1.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawLine(const glm::vec3& fromWorldSpacePosition, const glm::vec3& toWorldSpacePosition, const float sRgbColor[3], float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawScreenText(const char* text, const glm::vec2& screenSpacePixelPosition, const float sRgbColor[3], float scaling = 1.0f, float durationInSeconds = 0.0f);	// Add a 2D text string as an overlay to the current view, using a built-in font. Position origin at the top-left corner of the screen. Note: Newlines and tabs are handled (1 tab = 4 spaces).
		RENDERER_API_EXPORT void drawProjectedText(const char* text, const glm::vec3& worldSpacePosition, const float sRgbColor[3], const glm::mat4& worldSpaceToClipSpaceMatrix, const glm::ivec2& viewportPixelPosition, const glm::ivec2& viewportPixelSize, float scaling = 1.0f, float durationInSeconds = 0.0f);	// Add a 3D text label centered at the given world position that gets projected to screen-space. The label always faces the viewer.
		RENDERER_API_EXPORT void drawAxisTriad(const glm::mat4& objectSpaceToWorldSpaceMatrix, float arrowHeadSize, float arrowLength, float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);	// Add a set of three coordinate axis depicting the position and orientation of the given transform matrix
		RENDERER_API_EXPORT void drawArrow(const glm::vec3& fromWorldSpacePosition, const glm::vec3& toWorldSpacePosition, const float sRgbColor[3], float arrowHeadSize, float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawCross(const glm::vec3& worldSpaceCenter, float length, float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);	// Add an axis-aligned cross (3 lines converging at a point)
		RENDERER_API_EXPORT void drawCircle(const glm::vec3& worldSpaceCenter, const glm::vec3& normalizedWorldSpacePlaneNormal, const float sRgbColor[3], float radius, float numberOfSteps, float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawPlane(const glm::vec3& worldSpaceCenter, const glm::vec3& normalizedWorldSpacePlaneNormal, const float planeColor[3], const float normalColor[3], float planeScale, float normalScale, float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);	// If "normalScale" is not zero, a line depicting the plane normal is also drawn
		RENDERER_API_EXPORT void drawSphere(const glm::vec3& worldSpaceCenter, const float sRgbColor[3], float radius, float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawCone(const glm::vec3& worldSpaceApex, const glm::vec3& worldSpaceDirectionAndLength, const float sRgbColor[3], float baseRadius, float apexRadius, float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawBox(const glm::vec3 worldSpacePoints[8], const float sRgbColor[3], float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawBox(const glm::vec3& worldSpaceCenter, const glm::vec3& size, const float sRgbColor[3], float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawAabb(const glm::vec3& worldSpaceMinimumPosition, const glm::vec3& worldSpaceMaximumPosition, const float sRgbColor[3], float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawFrustum(const glm::mat4& clipSpaceToObjectSpace, const float sRgbColor[3], float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawVertexNormal(const glm::vec3& worldSpaceOrigin, const glm::vec3& normalizedWorldSpaceNormal, float length, float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);
		RENDERER_API_EXPORT void drawTangentBasis(const glm::vec3& worldSpaceOrigin, const glm::vec3& normalizedWorldSpaceNormal, const glm::vec3& normalizedWorldSpaceTangent, const glm::vec3& normalizedWorldSpaceBitangent, float lengths, float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);	// Color scheme used is: normal=WHITE, tangent=YELLOW, bi-tangent=MAGENTA
		RENDERER_API_EXPORT void drawXzSquareGrid(float worldSpaceMinimumXZPosition, float worldSpaceMaximumXZPosition, float worldSpaceYPosition, float stepSize, const float sRgbColor[3], float lineWidth = 2.0f, float durationInSeconds = 0.0f, bool depthEnabled = true);


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ISceneItem methods           ]
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

		[[nodiscard]] virtual const RenderableManager* getRenderableManager() const override;


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::IResourceListener methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void onLoadingStateChange(const IResource& resource) override;


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct MaterialData final
		{
			AssetId				materialAssetId;											///< If material blueprint asset ID is set, material asset ID must be invalid
			MaterialTechniqueId	materialTechniqueId = getInvalid<MaterialTechniqueId>();	///< Must always be valid
			AssetId				materialBlueprintAssetId;									///< If material asset ID is set, material blueprint asset ID must be invalid
			MaterialProperties	materialProperties;
			MaterialResourceId	materialResourceId = getInvalid<MaterialResourceId>();
		};


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit DebugDrawSceneItem(SceneResource& sceneResource);
		virtual ~DebugDrawSceneItem() override;
		explicit DebugDrawSceneItem(const DebugDrawSceneItem&) = delete;
		DebugDrawSceneItem& operator=(const DebugDrawSceneItem&) = delete;
		void initialize(RenderableIndex renderableIndex, MaterialData& materialData);
		void createMaterialResource(RenderableIndex renderableIndex, MaterialData& materialData, MaterialResourceId parentMaterialResourceId);


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		RenderableManager  mRenderableManager;
		MaterialData	   mMaterialData[RenderableIndex::NUMBER_OF_INDICES];
		MaterialResourceId mLoadingMaterialResourceIds[RenderableIndex::NUMBER_OF_INDICES];
		void*			   mDebugDrawRenderInterface;											// "::DebugDrawSceneItemDetail::DebugDrawRenderInterface"-type
		void*			   mContextHandle;														// "dd::ContextHandle"-type


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
