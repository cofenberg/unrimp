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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Renderer/Public/Resource/Scene/Item/Debug/DebugDrawSceneItem.h"
#include "Renderer/Public/Resource/Scene/SceneResource.h"
#include "Renderer/Public/Resource/Scene/SceneNode.h"
#include "Renderer/Public/Core/Time/TimeManager.h"
#include "Renderer/Public/IRenderer.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'initializing': conversion from 'const uint32_t' to 'int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(5219)	// warning C5219: implicit conversion from 'int' to 'float', possible loss of data
	#define DEBUG_DRAW_CXX11_SUPPORTED 1
	#define DEBUG_DRAW_NO_DEFAULT_COLORS
	#define DEBUG_DRAW_EXPLICIT_CONTEXT
	#define DEBUG_DRAW_IMPLEMENTATION
	#include <debug-draw/debug_draw.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace DebugDrawSceneItemDetail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static const Renderer::AssetId DEBUG_DRAW_GLYPH_MAP_2D(ASSET_ID("Unrimp/Texture/DynamicByCode/DebugDrawGlyphMap2D"));
		Rhi::ITexture2DPtr g_Texture2D;
		Renderer::TextureResourceId g_TextureResourceId = Renderer::getInvalid<Renderer::TextureResourceId>();
		uint32_t g_NumberOfGlyphTextureReferences = 0;


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		inline int secondsToMilliseconds(float seconds)
		{
			return static_cast<int>(seconds * 1000.0f);
		}


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		class DebugDrawRenderInterface : public dd::RenderInterface
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			inline DebugDrawRenderInterface(const Renderer::IRenderer& renderer, Renderer::RenderableManager& renderableManager) :
				mRhi(renderer.getRhi()),
				mRenderer(renderer),
				mRenderableManager(renderableManager)
			{
				Rhi::IBufferManager& bufferManager = renderer.getBufferManager();

				// Point list: The RHI implementation must support structured buffers
				if (renderer.getRhi().getCapabilities().maximumStructuredBufferSize > 0)
				{
					// Create the structured buffer
					for (int depthIndex = 0; depthIndex < 2; ++depthIndex)
					{
						mPointListStructuredBuffer[depthIndex] = bufferManager.createStructuredBuffer(sizeof(dd::DrawVertex) * DEBUG_DRAW_VERTEX_BUFFER_SIZE, nullptr, Rhi::BufferFlag::SHADER_RESOURCE, Rhi::BufferUsage::DYNAMIC_DRAW, sizeof(dd::DrawVertex) RHI_RESOURCE_DEBUG_NAME("DebugDrawPointList"));
					}
				}
				else
				{
					RHI_LOG_ONCE(renderer.getContext(), COMPATIBILITY_WARNING, "The renderer debug draw scene item needs a RHI implementation with structured buffer support for point rendering")
				}

				{ // Create line list vertex array object (VAO)
					// Vertex input layout for line list
					static constexpr Rhi::VertexAttribute vertexAttributesLayoutLineList[] =
					{
						{ // Attribute 0
							// Data destination
							Rhi::VertexAttributeFormat::FLOAT_4,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
							"Position",								// name[32] (char)
							"POSITION",								// semanticName[32] (char)
							0,										// semanticIndex (uint32_t)
							// Data source
							0,										// inputSlot (uint32_t)
							0,										// alignedByteOffset (uint32_t)
							sizeof(dd::DrawVertex),					// strideInBytes (uint32_t)
							0										// instancesPerElement (uint32_t)
						},
						{ // Attribute 1
							// Data destination
							Rhi::VertexAttributeFormat::FLOAT_4,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
							"Color",								// name[32] (char)
							"COLOR",								// semanticName[32] (char)
							0,										// semanticIndex (uint32_t)
							// Data source
							0,										// inputSlot (uint32_t)
							sizeof(float) * 4,						// alignedByteOffset (uint32_t)
							sizeof(dd::DrawVertex),					// strideInBytes (uint32_t)
							0										// instancesPerElement (uint32_t)
						}
					};
					const Rhi::VertexAttributes vertexAttributes(static_cast<uint32_t>(GLM_COUNTOF(vertexAttributesLayoutLineList)), vertexAttributesLayoutLineList);

					// Create line list vertex array object (VAO)
					for (int depthIndex = 0; depthIndex < 2; ++depthIndex)
					{
						mLineListVertexBuffer[depthIndex] = bufferManager.createVertexBuffer(sizeof(dd::DrawVertex) * DEBUG_DRAW_VERTEX_BUFFER_SIZE, nullptr, 0, Rhi::BufferUsage::DYNAMIC_DRAW RHI_RESOURCE_DEBUG_NAME("DebugDrawLineList"));
						const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { { mLineListVertexBuffer[depthIndex] } };
						mLineListVertexArray[depthIndex] = bufferManager.createVertexArray(vertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, nullptr RHI_RESOURCE_DEBUG_NAME("DebugDrawLineList"));
					}
				}

				{ // Create glyph list vertex array object (VAO)
					// Vertex input layout for glyph list
					static constexpr Rhi::VertexAttribute vertexAttributesLayoutLineList[] =
					{
						{ // Attribute 0
							// Data destination
							Rhi::VertexAttributeFormat::FLOAT_4,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
							"Position",								// name[32] (char)
							"POSITION",								// semanticName[32] (char)
							0,										// semanticIndex (uint32_t)
							// Data source
							0,										// inputSlot (uint32_t)
							0,										// alignedByteOffset (uint32_t)
							sizeof(float) * 4 + sizeof(float) * 4,	// strideInBytes (uint32_t)
							0										// instancesPerElement (uint32_t)
						},
						{ // Attribute 1
							// Data destination
							Rhi::VertexAttributeFormat::FLOAT_4,	// vertexAttributeFormat (Rhi::VertexAttributeFormat)
							"Color",								// name[32] (char)
							"COLOR",								// semanticName[32] (char)
							0,										// semanticIndex (uint32_t)
							// Data source
							0,										// inputSlot (uint32_t)
							sizeof(float) * 4,						// alignedByteOffset (uint32_t)
							sizeof(float) * 4 + sizeof(float) * 4,	// strideInBytes (uint32_t)
							0										// instancesPerElement (uint32_t)
						}
					};
					const Rhi::VertexAttributes vertexAttributes(static_cast<uint32_t>(GLM_COUNTOF(vertexAttributesLayoutLineList)), vertexAttributesLayoutLineList);

					// Create glyph list vertex array object (VAO)
					mGlyphListVertexBuffer = bufferManager.createVertexBuffer(sizeof(dd::DrawVertex) * DEBUG_DRAW_VERTEX_BUFFER_SIZE, nullptr, 0, Rhi::BufferUsage::DYNAMIC_DRAW RHI_RESOURCE_DEBUG_NAME("DebugDrawGlyphList"));
					const Rhi::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { { mGlyphListVertexBuffer } };
					mGlyphListVertexArray = bufferManager.createVertexArray(vertexAttributes, static_cast<uint32_t>(GLM_COUNTOF(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, nullptr RHI_RESOURCE_DEBUG_NAME("DebugDrawGlyphList"));
				}

				// Setup renderable manager
				#ifdef RHI_DEBUG
					const char* debugName = "DebugDraw";
					mRenderableManager.setDebugName(debugName);
				#endif
				const Renderer::MaterialResourceManager& materialResourceManager = renderer.getMaterialResourceManager();
				Renderer::RenderableManager::Renderables& renderables = mRenderableManager.getRenderables();
				renderables.reserve(Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES);
				renderables.emplace_back(mRenderableManager, renderer.getMeshResourceManager().getDrawIdVertexArrayPtr(), materialResourceManager, Renderer::getInvalid<Renderer::MaterialResourceId>(), Renderer::getInvalid<Renderer::SkeletonResourceId>(), false, 0, 6, 0 RHI_RESOURCE_DEBUG_NAME(debugName));	// Renderer::DebugDrawSceneItem::RenderableIndex::POINT_LIST_DEPTH_DISABLED
				renderables.emplace_back(mRenderableManager, renderer.getMeshResourceManager().getDrawIdVertexArrayPtr(), materialResourceManager, Renderer::getInvalid<Renderer::MaterialResourceId>(), Renderer::getInvalid<Renderer::SkeletonResourceId>(), false, 0, 6, 0 RHI_RESOURCE_DEBUG_NAME(debugName));	// Renderer::DebugDrawSceneItem::RenderableIndex::POINT_LIST_DEPTH_ENABLED
				renderables.emplace_back(mRenderableManager, mLineListVertexArray[0],									  materialResourceManager, Renderer::getInvalid<Renderer::MaterialResourceId>(), Renderer::getInvalid<Renderer::SkeletonResourceId>(), false, 0, 0, 1 RHI_RESOURCE_DEBUG_NAME(debugName));	// Renderer::DebugDrawSceneItem::RenderableIndex::LINE_LIST_DEPTH_DISABLED
				renderables.emplace_back(mRenderableManager, mLineListVertexArray[1],									  materialResourceManager, Renderer::getInvalid<Renderer::MaterialResourceId>(), Renderer::getInvalid<Renderer::SkeletonResourceId>(), false, 0, 0, 1 RHI_RESOURCE_DEBUG_NAME(debugName));	// Renderer::DebugDrawSceneItem::RenderableIndex::LINE_LIST_DEPTH_ENABLED
				renderables.emplace_back(mRenderableManager, mGlyphListVertexArray,										  materialResourceManager, Renderer::getInvalid<Renderer::MaterialResourceId>(), Renderer::getInvalid<Renderer::SkeletonResourceId>(), false, 0, 0, 1 RHI_RESOURCE_DEBUG_NAME(debugName));	// Renderer::DebugDrawSceneItem::RenderableIndex::GLYPH_LIST
				mRenderableManager.updateCachedRenderablesData();
			}

			void clear()
			{
				Renderer::RenderableManager::Renderables& renderables = mRenderableManager.getRenderables();
				renderables[Renderer::DebugDrawSceneItem::RenderableIndex::POINT_LIST_DEPTH_DISABLED].setInstanceCount(0);
				renderables[Renderer::DebugDrawSceneItem::RenderableIndex::POINT_LIST_DEPTH_ENABLED].setInstanceCount(0);
				for (uint32_t i = 2; i < Renderer::DebugDrawSceneItem::RenderableIndex::NUMBER_OF_INDICES; ++i)
				{
					renderables[i].setNumberOfIndices(0);
				}
			}

			void onMaterialResourceCreated(const Renderer::MaterialResourceManager& materialResourceManager, Renderer::DebugDrawSceneItem::RenderableIndex renderableIndex, Renderer::MaterialResourceId materialResourceId)
			{
				mRenderableManager.getRenderables()[renderableIndex].setMaterialResourceId(materialResourceManager, materialResourceId);
				mRenderableManager.updateCachedRenderablesData();

				// Tell the used material resource about our structured buffer
				if (Renderer::DebugDrawSceneItem::RenderableIndex::POINT_LIST_DEPTH_DISABLED == renderableIndex && nullptr != mPointListStructuredBuffer[0])
				{
					for (Renderer::MaterialTechnique* materialTechnique : materialResourceManager.getById(materialResourceId).getSortedMaterialTechniqueVector())
					{
						materialTechnique->setStructuredBufferPtr(2, mPointListStructuredBuffer[0]);
					}
				}
				else if (Renderer::DebugDrawSceneItem::RenderableIndex::POINT_LIST_DEPTH_ENABLED == renderableIndex && nullptr != mPointListStructuredBuffer[1])
				{
					for (Renderer::MaterialTechnique* materialTechnique : materialResourceManager.getById(materialResourceId).getSortedMaterialTechniqueVector())
					{
						materialTechnique->setStructuredBufferPtr(2, mPointListStructuredBuffer[1]);
					}
				}
			}


		//[-------------------------------------------------------]
		//[ Public virtual dd::RenderInterface methods            ]
		//[-------------------------------------------------------]
		public:
			virtual dd::GlyphTextureHandle createGlyphTexture(int width, int height, const void* pixels) override
			{
				// Sanity checks
				RHI_ASSERT(mRhi.getContext(), nullptr != pixels, "Invalid pixels pointer")
				RHI_ASSERT(mRhi.getContext(), width > 0 && width <= DEBUG_DRAW_VERTEX_BUFFER_SIZE, "Invalid width")
				RHI_ASSERT(mRhi.getContext(), height > 0 && height <= DEBUG_DRAW_VERTEX_BUFFER_SIZE, "Invalid height")

				// We use one debug-draw context per debug draw scene item, but we don't need to have one and the same glyph texture multiple times in memory
				if (nullptr == g_Texture2D)
				{
					g_NumberOfGlyphTextureReferences = 1;

					// Upload texture to RHI
					g_Texture2D = mRenderer.getTextureManager().createTexture2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height), Rhi::TextureFormat::R8, pixels, Rhi::TextureFlag::GENERATE_MIPMAPS | Rhi::TextureFlag::SHADER_RESOURCE, Rhi::TextureUsage::DEFAULT, 1, nullptr RHI_RESOURCE_DEBUG_NAME("Debug draw 2D GUI glyph texture atlas"));

					// Tell the texture resource manager about our glyph texture so it can be referenced inside e.g. compositor nodes
					g_TextureResourceId = mRenderer.getTextureResourceManager().createTextureResourceByAssetId(DEBUG_DRAW_GLYPH_MAP_2D, *g_Texture2D);
				}
				else
				{
					++g_NumberOfGlyphTextureReferences;
				}

				// Done
				return reinterpret_cast<dd::GlyphTextureHandle>(g_Texture2D.getPointer());
			}

			virtual void destroyGlyphTexture(dd::GlyphTextureHandle) override
			{
				// "dd::GlyphTextureHandle" is unused by intent, we only support a single glyph texture for all debug draw context instances

				// We use one debug-draw context per debug draw scene item, but we don't need to have one and the same glyph texture multiple times in memory
				RHI_ASSERT(mRhi.getContext(), g_NumberOfGlyphTextureReferences > 0, "Invalid number of glyph texture references")
				--g_NumberOfGlyphTextureReferences;
				if (0 == g_NumberOfGlyphTextureReferences)
				{
					mRenderer.getTextureResourceManager().destroyTextureResource(g_TextureResourceId);
					g_Texture2D = nullptr;
				}
			}

			virtual void drawPointList(const dd::DrawVertex* points, int count, bool depthEnabled) override
			{
				// Sanity checks
				RHI_ASSERT(mRhi.getContext(), nullptr != points, "Invalid points pointer")
				RHI_ASSERT(mRhi.getContext(), count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE, "Invalid count")
				RHI_ASSERT(mRhi.getContext(), !mRenderableManager.getRenderables().empty(), "Invalid renderables")

				// Structured buffer might not be supported by the used RHI, so we need to check for it
				const uint32_t depthIndex = (depthEnabled ? 1u : 0u);
				if (nullptr != mPointListStructuredBuffer[depthIndex])
				{
					{ // Copy all points into a single contiguous buffer
						Rhi::MappedSubresource structuredBufferMappedSubresource;
						if (mRhi.map(*mPointListStructuredBuffer[depthIndex], 0, Rhi::MapType::WRITE_DISCARD, 0, structuredBufferMappedSubresource))
						{
							memcpy(structuredBufferMappedSubresource.data, points, sizeof(dd::DrawVertex) * count);

							// Unmap the structured buffer
							mRhi.unmap(*mPointListStructuredBuffer[depthIndex], 0);
						}
					}

					// Update the instance count of the point list renderable
					mRenderableManager.getRenderables()[Renderer::DebugDrawSceneItem::RenderableIndex::POINT_LIST_DEPTH_DISABLED + depthIndex].setInstanceCount(static_cast<uint32_t>(count));
				}
			}

			virtual void drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled) override
			{
				// Sanity checks
				RHI_ASSERT(mRhi.getContext(), nullptr != lines, "Invalid lines pointer")
				RHI_ASSERT(mRhi.getContext(), count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE, "Invalid count")
				RHI_ASSERT(mRhi.getContext(), !mRenderableManager.getRenderables().empty(), "Invalid renderables")

				const uint32_t depthIndex = (depthEnabled ? 1u : 0u);
				{ // Copy all vertices into a single contiguous buffer
					Rhi::MappedSubresource vertexBufferMappedSubresource;
					if (mRhi.map(*mLineListVertexBuffer[depthIndex], 0, Rhi::MapType::WRITE_DISCARD, 0, vertexBufferMappedSubresource))
					{
						memcpy(vertexBufferMappedSubresource.data, lines, sizeof(dd::DrawVertex) * count);

						// Unmap the vertex buffer
						mRhi.unmap(*mLineListVertexBuffer[depthIndex], 0);
					}
				}

				// Update the number of indices of the line list renderable
				mRenderableManager.getRenderables()[Renderer::DebugDrawSceneItem::RenderableIndex::LINE_LIST_DEPTH_DISABLED + depthIndex].setNumberOfIndices(static_cast<uint32_t>(count));
			}

			virtual void drawGlyphList(const dd::DrawVertex* glyphs, int count, dd::GlyphTextureHandle) override
			{
				// "dd::GlyphTextureHandle" is unused by intent, we only support a single glyph texture for all debug draw context instances

				// Sanity checks
				RHI_ASSERT(mRhi.getContext(), nullptr != glyphs, "Invalid glyph pointer")
				RHI_ASSERT(mRhi.getContext(), count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE, "Invalid count")
				RHI_ASSERT(mRhi.getContext(), !mRenderableManager.getRenderables().empty(), "Invalid renderables")

				{ // Copy all vertices into a single contiguous buffer
					Rhi::MappedSubresource vertexBufferMappedSubresource;
					if (mRhi.map(*mGlyphListVertexBuffer, 0, Rhi::MapType::WRITE_DISCARD, 0, vertexBufferMappedSubresource))
					{
						memcpy(vertexBufferMappedSubresource.data, glyphs, sizeof(dd::DrawVertex) * count);

						// Unmap the vertex buffer
						mRhi.unmap(*mGlyphListVertexBuffer, 0);
					}
				}

				// Update the number of indices of the glyph list renderable
				mRenderableManager.getRenderables()[Renderer::DebugDrawSceneItem::RenderableIndex::GLYPH_LIST].setNumberOfIndices(static_cast<uint32_t>(count));
			}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			explicit DebugDrawRenderInterface(const DebugDrawRenderInterface&) = delete;
			DebugDrawRenderInterface& operator=(const DebugDrawRenderInterface&) = delete;


		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Rhi::IRhi&					 mRhi;
			const Renderer::IRenderer&   mRenderer;
			Renderer::RenderableManager& mRenderableManager;
			Rhi::IStructuredBufferPtr	 mPointListStructuredBuffer[2];	///< Structured buffer the data of the individual points ("dd::DrawVertex")
			Rhi::IVertexBufferPtr		 mLineListVertexBuffer[2];		///< Line list vertex buffer object (VBO), can be a null pointer, "Renderer::DebugDrawSceneItem::RenderableIndex::LINE_LIST"
			Rhi::IVertexArrayPtr		 mLineListVertexArray[2];		///< Line list vertex array object (VAO), can be a null pointer, "Renderer::DebugDrawSceneItem::RenderableIndex::LINE_LIST"
			Rhi::IVertexBufferPtr		 mGlyphListVertexBuffer;		///< Glyph list vertex buffer object (VBO), can be a null pointer, "Renderer::DebugDrawSceneItem::RenderableIndex::GLYPH_LIST"
			Rhi::IVertexArrayPtr		 mGlyphListVertexArray;			///< Glyph list vertex array object (VAO), can be a null pointer, "Renderer::DebugDrawSceneItem::RenderableIndex::GLYPH_LIST"


		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // DebugDrawSceneItemDetail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	void DebugDrawSceneItem::getDefaultTextureAssetIds(AssetIds& assetIds)
	{
		assetIds.push_back(::DebugDrawSceneItemDetail::DEBUG_DRAW_GLYPH_MAP_2D);
	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	void DebugDrawSceneItem::clear()
	{
		static_cast<::DebugDrawSceneItemDetail::DebugDrawRenderInterface*>(mDebugDrawRenderInterface)->clear();
		dd::clear(static_cast<dd::ContextHandle>(mContextHandle));
	}

	void DebugDrawSceneItem::flush()
	{
		dd::flush(static_cast<dd::ContextHandle>(mContextHandle), static_cast<int64_t>(getSceneResource().getRenderer().getTimeManager().getSinceStartStopwatch().getMilliseconds()));
	}

	void DebugDrawSceneItem::drawPoint(const glm::vec3& worldSpacePosition, const float sRgbColor[3], float size, float durationInSeconds, bool depthEnabled)
	{
		dd::point(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpacePosition), sRgbColor, size, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawLine(const glm::vec3& fromWorldSpacePosition, const glm::vec3& toWorldSpacePosition, const float sRgbColor[3], float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::line(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(fromWorldSpacePosition), glm::value_ptr(toWorldSpacePosition), sRgbColor, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawScreenText(const char* text, const glm::vec2& screenSpacePixelPosition, const float sRgbColor[3], float scaling, float durationInSeconds)
	{
		// "dd::screenText()" is using a three component position but ignores z, to avoid any confusion "DebugDrawSceneItem::drawScreenText()" is using a two component position
		dd::screenText(static_cast<dd::ContextHandle>(mContextHandle), text, glm::value_ptr(screenSpacePixelPosition), sRgbColor, scaling, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds));
	}

	void DebugDrawSceneItem::drawProjectedText(const char* text, const glm::vec3& worldSpacePosition, const float sRgbColor[3], const glm::mat4& worldSpaceToClipSpaceMatrix, const glm::ivec2& viewportPixelPosition, const glm::ivec2& viewportPixelSize, float scaling, float durationInSeconds)
	{
		dd::projectedText(static_cast<dd::ContextHandle>(mContextHandle), text, glm::value_ptr(worldSpacePosition), sRgbColor, glm::value_ptr(worldSpaceToClipSpaceMatrix), viewportPixelPosition.x, viewportPixelPosition.y, viewportPixelSize.x, viewportPixelSize.y, scaling, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds));
	}

	void DebugDrawSceneItem::drawAxisTriad(const glm::mat4& objectSpaceToWorldSpaceMatrix, float arrowHeadSize, float arrowLength, float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::axisTriad(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(objectSpaceToWorldSpaceMatrix), arrowHeadSize, arrowLength, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawArrow(const glm::vec3& fromWorldSpacePosition, const glm::vec3& toWorldSpacePosition, const float sRgbColor[3], float arrowHeadSize, float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::arrow(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(fromWorldSpacePosition), glm::value_ptr(toWorldSpacePosition), sRgbColor, arrowHeadSize, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawCross(const glm::vec3& worldSpaceCenter, float length, float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::cross(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpaceCenter), length, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawCircle(const glm::vec3& worldSpaceCenter, const glm::vec3& normalizedWorldSpacePlaneNormal, const float sRgbColor[3], float radius, float numberOfSteps, float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::circle(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpaceCenter), glm::value_ptr(normalizedWorldSpacePlaneNormal), sRgbColor, radius, numberOfSteps, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawPlane(const glm::vec3& worldSpaceCenter, const glm::vec3& normalizedWorldSpacePlaneNormal, const float planeColor[3], const float normalColor[3], float planeScale, float normalScale, float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::plane(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpaceCenter), glm::value_ptr(normalizedWorldSpacePlaneNormal), planeColor, normalColor, planeScale, normalScale, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawSphere(const glm::vec3& worldSpaceCenter, const float sRgbColor[3], float radius, float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::sphere(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpaceCenter), sRgbColor, radius, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawCone(const glm::vec3& worldSpaceApex, const glm::vec3& worldSpaceDirectionAndLength, const float sRgbColor[3], float baseRadius, float apexRadius, float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::cone(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpaceApex), glm::value_ptr(worldSpaceDirectionAndLength), sRgbColor, baseRadius, apexRadius, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawBox(const glm::vec3 worldSpacePoints[8], const float sRgbColor[3], float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		// Copied from "dd::box()" implementation
		const int durationInMilliseconds = ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds);

		// Build the lines from points using clever indexing tricks:
		// (& 3 is a fancy way of doing % 4, but avoids the expensive modulo operation)
		for (int i = 0; i < 4; ++i)
		{
			dd::line(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpacePoints[i]), glm::value_ptr(worldSpacePoints[(i + 1) & 3]), sRgbColor, lineWidth, durationInMilliseconds, depthEnabled);
			dd::line(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpacePoints[4 + i]), glm::value_ptr(worldSpacePoints[4 + ((i + 1) & 3)]), sRgbColor, lineWidth, durationInMilliseconds, depthEnabled);
			dd::line(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpacePoints[i]), glm::value_ptr(worldSpacePoints[4 + i]), sRgbColor, lineWidth, durationInMilliseconds, depthEnabled);
		}
	}

	void DebugDrawSceneItem::drawBox(const glm::vec3& worldSpaceCenter, const glm::vec3& size, const float sRgbColor[3], float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::box(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpaceCenter), sRgbColor, size.x, size.y, size.z, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawAabb(const glm::vec3& worldSpaceMinimumPosition, const glm::vec3& worldSpaceMaximumPosition, const float sRgbColor[3], float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::aabb(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpaceMinimumPosition), glm::value_ptr(worldSpaceMaximumPosition), sRgbColor, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawFrustum(const glm::mat4& clipSpaceToObjectSpace, const float sRgbColor[3], float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		// "dd::frustum()" is using a reverse projection as mentioned in "Three Methods to Extract Frustum Points" by Don Williamson online at http://donw.io/post/frustum-point-extraction/ (see article for alternative solutions)
		dd::frustum(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(clipSpaceToObjectSpace), sRgbColor, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawVertexNormal(const glm::vec3& worldSpaceOrigin, const glm::vec3& normalizedWorldSpaceNormal, float length, float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::vertexNormal(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpaceOrigin), glm::value_ptr(normalizedWorldSpaceNormal), length, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawTangentBasis(const glm::vec3& worldSpaceOrigin, const glm::vec3& normalizedWorldSpaceNormal, const glm::vec3& normalizedWorldSpaceTangent, const glm::vec3& normalizedWorldSpaceBitangent, float lengths, float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::tangentBasis(static_cast<dd::ContextHandle>(mContextHandle), glm::value_ptr(worldSpaceOrigin), glm::value_ptr(normalizedWorldSpaceNormal), glm::value_ptr(normalizedWorldSpaceTangent), glm::value_ptr(normalizedWorldSpaceBitangent), lengths, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}

	void DebugDrawSceneItem::drawXzSquareGrid(float worldSpaceMinimumXZPosition, float worldSpaceMaximumXZPosition, float worldSpaceYPosition, float stepSize, const float sRgbColor[3], float lineWidth, float durationInSeconds, bool depthEnabled)
	{
		dd::xzSquareGrid(static_cast<dd::ContextHandle>(mContextHandle), worldSpaceMinimumXZPosition, worldSpaceMaximumXZPosition, worldSpaceYPosition, stepSize, sRgbColor, lineWidth, ::DebugDrawSceneItemDetail::secondsToMilliseconds(durationInSeconds), depthEnabled);
	}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ISceneItem methods           ]
	//[-------------------------------------------------------]
	void DebugDrawSceneItem::deserialize([[maybe_unused]] uint32_t numberOfBytes, const uint8_t* data)
	{
		// Sanity check
		RHI_ASSERT(getContext(), sizeof(v1Scene::DebugDrawItem) <= numberOfBytes, "Invalid number of bytes")

		const uint8_t* currentData = data;
		for (uint32_t i = 0; i < RenderableIndex::NUMBER_OF_INDICES; ++i)
		{
			MaterialData& materialData = mMaterialData[i];

			// Read data
			const v1Scene::MaterialData* v1SceneMaterialData = reinterpret_cast<const v1Scene::MaterialData*>(currentData);
			RHI_ASSERT(getContext(), sizeof(v1Scene::MaterialData) + sizeof(MaterialProperty) * v1SceneMaterialData->numberOfMaterialProperties <= numberOfBytes, "Invalid number of bytes")
			materialData.materialAssetId = v1SceneMaterialData->materialAssetId;
			materialData.materialTechniqueId = v1SceneMaterialData->materialTechniqueId;
			materialData.materialBlueprintAssetId = v1SceneMaterialData->materialBlueprintAssetId;

			{ // Read material properties
				// TODO(co) Get rid of the evil const-cast
				MaterialProperties::SortedPropertyVector& sortedPropertyVector = const_cast<MaterialProperties::SortedPropertyVector&>(materialData.materialProperties.getSortedPropertyVector());
				sortedPropertyVector.resize(v1SceneMaterialData->numberOfMaterialProperties);
				memcpy(reinterpret_cast<char*>(sortedPropertyVector.data()), currentData + sizeof(v1Scene::MaterialData), sizeof(MaterialProperty) * v1SceneMaterialData->numberOfMaterialProperties);
			}

			// Advance current data pointer
			currentData += sizeof(v1Scene::MaterialData) + sizeof(MaterialProperty) * v1SceneMaterialData->numberOfMaterialProperties;

			// Sanity checks
			RHI_ASSERT(getContext(), isValid(materialData.materialAssetId) || isValid(materialData.materialBlueprintAssetId), "Invalid data")
			RHI_ASSERT(getContext(), !(isValid(materialData.materialAssetId) && isValid(materialData.materialBlueprintAssetId)), "Invalid data")
		}
	}

	void DebugDrawSceneItem::onAttachedToSceneNode(SceneNode& sceneNode)
	{
		mRenderableManager.setTransform(&sceneNode.getGlobalTransform());

		// Call the base implementation
		ISceneItem::onAttachedToSceneNode(sceneNode);
	}

	const RenderableManager* DebugDrawSceneItem::getRenderableManager() const
	{
		// Sanity checks
		RHI_ASSERT(getContext(), Math::QUAT_IDENTITY == mRenderableManager.getTransform().rotation, "No rotation is supported to keep things simple")
		RHI_ASSERT(getContext(), Math::VEC3_ONE == mRenderableManager.getTransform().scale, "No scale is supported to keep things simple")

		for (uint32_t i = 0; i < RenderableIndex::NUMBER_OF_INDICES; ++i)
		{
			if (!isValid(mMaterialData[i].materialResourceId))
			{
				// TODO(co) Get rid of the nasty delayed initialization in here, including the evil const-cast. For this, full asynchronous material blueprint loading must work. See "TODO(co) Currently material blueprint resource loading is a blocking process.".
				const_cast<DebugDrawSceneItem*>(this)->initialize(static_cast<RenderableIndex>(i), const_cast<MaterialData&>(mMaterialData[i]));
			}
		}

		return &mRenderableManager;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::IResourceListener methods ]
	//[-------------------------------------------------------]
	void DebugDrawSceneItem::onLoadingStateChange(const IResource& resource)
	{
		if (resource.getLoadingState() == IResource::LoadingState::LOADED)
		{
			for (uint32_t i = 0; i < RenderableIndex::NUMBER_OF_INDICES; ++i)
			{
				MaterialData& materialData = mMaterialData[i];
				if (mLoadingMaterialResourceIds[i] == resource.getId())
				{
					setInvalid(mLoadingMaterialResourceIds[i]);

					// Destroy the material resource we created
					if (isValid(materialData.materialResourceId))
					{
						getSceneResource().getRenderer().getMaterialResourceManager().destroyMaterialResource(materialData.materialResourceId);
						setInvalid(materialData.materialResourceId);
					}

					// Create material resource
					createMaterialResource(static_cast<RenderableIndex>(i), materialData, resource.getId());
					return;
				}
			}
			RHI_ASSERT(getContext(), false, "Invalid asset ID")
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	DebugDrawSceneItem::DebugDrawSceneItem(SceneResource& sceneResource) :
		ISceneItem(sceneResource, false),	///< The debug draw isn't allowed to be culled
		mLoadingMaterialResourceIds{ getInvalid<MaterialResourceId>(), getInvalid<MaterialResourceId>(), getInvalid<MaterialResourceId>(), getInvalid<MaterialResourceId>(), getInvalid<MaterialResourceId>() },
		mDebugDrawRenderInterface(new ::DebugDrawSceneItemDetail::DebugDrawRenderInterface(getSceneResource().getRenderer(), mRenderableManager)),
		mContextHandle(nullptr)
	{
		// Initialize the debug-draw library
		dd::ContextHandle contextHandle = nullptr;
		dd::initialize(&contextHandle, static_cast<dd::RenderInterface*>(mDebugDrawRenderInterface));
		mContextHandle = contextHandle;
	}

	DebugDrawSceneItem::~DebugDrawSceneItem()
	{
		// Clear the renderable manager right now
		mRenderableManager.getRenderables().clear();

		// Shutdown the debug-draw library
		dd::shutdown(static_cast<dd::ContextHandle>(mContextHandle));
		delete static_cast<::DebugDrawSceneItemDetail::DebugDrawRenderInterface*>(mDebugDrawRenderInterface);

		// Destroy the material resources we created
		MaterialResourceManager& materialResourceManager = getSceneResource().getRenderer().getMaterialResourceManager();
		for (uint32_t i = 0; i < RenderableIndex::NUMBER_OF_INDICES; ++i)
		{
			const MaterialResourceId materialResourceId = mMaterialData[i].materialResourceId;
			if (isValid(materialResourceId))
			{
				materialResourceManager.destroyMaterialResource(materialResourceId);
			}
		}
	}

	void DebugDrawSceneItem::initialize(RenderableIndex renderableIndex, MaterialData& materialData)
	{
		// Sanity checks
		RHI_ASSERT(getContext(), isValid(materialData.materialAssetId) || isValid(materialData.materialBlueprintAssetId), "Invalid data")
		RHI_ASSERT(getContext(), !(isValid(materialData.materialAssetId) && isValid(materialData.materialBlueprintAssetId)), "Invalid data")

		// Get parent material resource ID and initiate creating the material resource
		MaterialResourceManager& materialResourceManager = getSceneResource().getRenderer().getMaterialResourceManager();
		if (isValid(materialData.materialAssetId))
		{
			// Get or load material resource
			MaterialResourceId materialResourceId = getInvalid<MaterialResourceId>();
			materialResourceManager.loadMaterialResourceByAssetId(materialData.materialAssetId, materialResourceId, this);
			mLoadingMaterialResourceIds[renderableIndex] = materialResourceId;
		}
		else
		{
			// Get or load material blueprint resource
			const AssetId materialBlueprintAssetId = materialData.materialBlueprintAssetId;
			if (isValid(materialBlueprintAssetId))
			{
				MaterialResourceId parentMaterialResourceId = materialResourceManager.getMaterialResourceIdByAssetId(materialBlueprintAssetId);
				if (isInvalid(parentMaterialResourceId))
				{
					parentMaterialResourceId = materialResourceManager.createMaterialResourceByAssetId(materialBlueprintAssetId, materialBlueprintAssetId, materialData.materialTechniqueId);
				}
				createMaterialResource(renderableIndex, materialData, parentMaterialResourceId);
			}
		}
	}

	void DebugDrawSceneItem::createMaterialResource(RenderableIndex renderableIndex, MaterialData& materialData, MaterialResourceId parentMaterialResourceId)
	{
		// Sanity checks
		RHI_ASSERT(getContext(), isInvalid(materialData.materialResourceId), "Invalid data")
		RHI_ASSERT(getContext(), isValid(parentMaterialResourceId), "Invalid data")

		// Each material user instance must have its own material resource since material property values might vary
		MaterialResourceManager& materialResourceManager = getSceneResource().getRenderer().getMaterialResourceManager();
		materialData.materialResourceId = materialResourceManager.createMaterialResourceByCloning(parentMaterialResourceId);

		{ // Set material properties
			const MaterialProperties::SortedPropertyVector& sortedPropertyVector = materialData.materialProperties.getSortedPropertyVector();
			if (!sortedPropertyVector.empty())
			{
				MaterialResource& materialResource = materialResourceManager.getById(materialData.materialResourceId);
				for (const MaterialProperty& materialProperty : sortedPropertyVector)
				{
					if (materialProperty.isOverwritten())
					{
						materialResource.setPropertyById(materialProperty.getMaterialPropertyId(), materialProperty, materialProperty.getUsage());
					}
				}
			}
		}

		// Tell the world debug draw render interface
		static_cast<::DebugDrawSceneItemDetail::DebugDrawRenderInterface*>(mDebugDrawRenderInterface)->onMaterialResourceCreated(materialResourceManager, renderableIndex, materialData.materialResourceId);
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
