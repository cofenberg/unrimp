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
#include "Renderer/Public/DebugGui/DebugGuiHelper.h"
#include "Renderer/Public/Core/Math/Transform.h"
#include "Renderer/Public/Resource/Scene/SceneNode.h"
#include "Renderer/Public/Resource/Scene/SceneResource.h"
#include "Renderer/Public/Resource/Scene/Item/Camera/CameraSceneItem.h"
#include "Renderer/Public/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.h"
#include "Renderer/Public/Resource/Skeleton/SkeletonResourceManager.h"
#include "Renderer/Public/Resource/Skeleton/SkeletonResource.h"
#include "Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h"
#include "Renderer/Public/IRenderer.h"

#include <ImGui/imgui.h>

#include <ImGuizmo/ImGuizmo.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	#include <glm/gtc/type_ptr.hpp>
	#include <glm/gtx/euler_angles.hpp>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	#include <string>
	#include <unordered_set>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static const ImVec4 GREEN_COLOR(0.0f, 1.0f, 0.0f, 1.0f);
		static const ImVec4 YELLOW_COLOR(1.0f, 1.0f, 0.0f, 1.0f);
		static const ImVec4 RED_COLOR(1.0f, 0.0f, 0.0f, 1.0f);


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		[[nodiscard]] bool objectSpaceToScreenSpacePosition(const glm::vec3& objectSpacePosition, const glm::mat4& objectSpaceToClipSpaceMatrix, ImVec2& screenSpacePosition)
		{
			glm::vec4 position = objectSpaceToClipSpaceMatrix * glm::vec4(objectSpacePosition, 1.0f);
			const bool inFront = (position.z >= 0.0f);
			position *= 0.5f / position.w;
			position += glm::vec4(0.5f, 0.5f, 0.0f, 0.0f);
			position.y = 1.0f - position.y;
			const ImGuiIO& imGuiIO = ImGui::GetIO();
			position.x *= imGuiIO.DisplaySize.x;
			position.y *= imGuiIO.DisplaySize.y;
			screenSpacePosition = ImVec2(position.x, position.y);

			// In front of camera?
			return inFront;
		}

		void draw3DLine(const glm::mat4& objectSpaceToClipSpaceMatrix, const glm::vec3& objectSpaceStartPosition, const glm::vec3& objectSpaceEndPosition, const ImColor& color, float thickness, ImDrawList& imDrawList)
		{
			// TODO(co) Add near plane clip
			ImVec2 screenSpaceStartPosition;
			ImVec2 screenSpaceEndPosition;
			const bool screenSpaceStartPositionVisible = objectSpaceToScreenSpacePosition(objectSpaceStartPosition, objectSpaceToClipSpaceMatrix, screenSpaceStartPosition);
			const bool screenSpaceEndPositionVisible = objectSpaceToScreenSpacePosition(objectSpaceEndPosition, objectSpaceToClipSpaceMatrix, screenSpaceEndPosition);
			if (screenSpaceStartPositionVisible || screenSpaceEndPositionVisible)
			{
				imDrawList.AddLine(screenSpaceStartPosition, screenSpaceEndPosition, color, thickness);
			}
		}

		// Basing on https://stackoverflow.com/a/33390058
		const char* stringFormatCommas(uint64_t number, char* output)
		{
			char* outputBackup = output;	// Backup pointer for return
			char buffer[100];
			snprintf(buffer, 100, "%I64d", number);
			size_t c = 2 - (strlen(buffer) % 3);

			for (const char* p = buffer; 0 != *p; ++p)
			{
				*output++ = *p;
				if (1 == c)
				{
					*output++ = '.';
				}
				c = (c + 1) % 3;
			}
			*--output = 0;

			return outputBackup;
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Private static data                                   ]
	//[-------------------------------------------------------]
	uint32_t DebugGuiHelper::mDrawTextCounter = 0;


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	void DebugGuiHelper::drawText(const char* text, float x, float y, bool drawBackground)
	{
		if (!drawBackground)
		{
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 0.0f, 0.0f, 0.0f));
		}
		ImGui::Begin(("Renderer::DebugGuiManager::drawText_" + std::to_string(mDrawTextCounter)).c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing);
			ImGui::Text("%s", text);
			ImGui::SetWindowPos(ImVec2(x, y));
		ImGui::End();
		if (!drawBackground)
		{
			ImGui::PopStyleColor();
		}
		++mDrawTextCounter;
	}

	void DebugGuiHelper::drawGizmo(const CameraSceneItem& cameraSceneItem, GizmoSettings& gizmoSettings, Transform& transform)
	{
		// Setup ImGuizmo
		if (ImGui::RadioButton("Translate", gizmoSettings.currentGizmoOperation == GizmoOperation::TRANSLATE))
		{
			gizmoSettings.currentGizmoOperation = GizmoOperation::TRANSLATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", gizmoSettings.currentGizmoOperation == GizmoOperation::ROTATE))
		{
			gizmoSettings.currentGizmoOperation = GizmoOperation::ROTATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", gizmoSettings.currentGizmoOperation == GizmoOperation::SCALE))
		{
			gizmoSettings.currentGizmoOperation = GizmoOperation::SCALE;
		}
		{ // Show and edit rotation quaternion using Euler angles in degree
			glm::vec3 eulerAngles;
			glm::extractEulerAngleYXZ(glm::mat4_cast(transform.rotation), eulerAngles.x, eulerAngles.y, eulerAngles.z);
			eulerAngles = glm::degrees(eulerAngles);
			{ // TODO(co) We're using a 64 bit position, ImGui can only process 32 bit floating point values
				glm::vec3 position = transform.position;
				ImGui::InputFloat3("Tr", glm::value_ptr(position), "%.3f");
				transform.position = position;
			}
			ImGui::InputFloat3("Rt", glm::value_ptr(eulerAngles), "%.3f");
			ImGui::InputFloat3("Sc", glm::value_ptr(transform.scale), "%.3f");
			transform.rotation = glm::eulerAngleYXZ(glm::radians(eulerAngles.x), glm::radians(eulerAngles.y), glm::radians(eulerAngles.z));
		}
		if (gizmoSettings.currentGizmoOperation != GizmoOperation::SCALE)
		{
			if (ImGui::RadioButton("Local", gizmoSettings.currentGizmoMode == GizmoMode::LOCAL))
			{
				gizmoSettings.currentGizmoMode = GizmoMode::LOCAL;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("World", gizmoSettings.currentGizmoMode == GizmoMode::WORLD))
			{
				gizmoSettings.currentGizmoMode = GizmoMode::WORLD;
			}
		}
		ImGui::Checkbox("", &gizmoSettings.useSnap);
		ImGui::SameLine();
		switch (gizmoSettings.currentGizmoOperation)
		{
			case GizmoOperation::TRANSLATE:
				ImGui::InputFloat3("Snap", &gizmoSettings.snap[0]);
				break;

			case GizmoOperation::ROTATE:
				ImGui::InputFloat("Angle Snap", &gizmoSettings.snap[0]);
				break;

			case GizmoOperation::SCALE:
				ImGui::InputFloat("Scale Snap", &gizmoSettings.snap[0]);
				break;

			case GizmoOperation::TRANSLATE_X:
			case GizmoOperation::TRANSLATE_Y:
			case GizmoOperation::TRANSLATE_Z:
			case GizmoOperation::ROTATE_X:
			case GizmoOperation::ROTATE_Y:
			case GizmoOperation::ROTATE_Z:
			case GizmoOperation::ROTATE_SCREEN:
			case GizmoOperation::SCALE_X:
			case GizmoOperation::SCALE_Y:
			case GizmoOperation::SCALE_Z:
			case GizmoOperation::BOUNDS:
			case GizmoOperation::SCALE_XU:
			case GizmoOperation::SCALE_YU:
			case GizmoOperation::SCALE_ZU:
			case GizmoOperation::SCALEU:
			case GizmoOperation::UNIVERSAL:
				break;
		}
		{ // Let ImGuizmo do its thing
			glm::mat4 matrix;
			transform.position -= cameraSceneItem.getWorldSpaceCameraPosition();	// Camera relative rendering
			transform.getAsMatrix(matrix);
			const ImGuizmo::OPERATION operation = static_cast<ImGuizmo::OPERATION>(gizmoSettings.currentGizmoOperation);
			const ImGuizmo::MODE mode = (ImGuizmo::SCALE == operation) ? ImGuizmo::LOCAL : static_cast<ImGuizmo::MODE>(gizmoSettings.currentGizmoMode);
			const ImGuiIO& imGuiIO = ImGui::GetIO();
			ImGuizmo::SetRect(0, 0, imGuiIO.DisplaySize.x, imGuiIO.DisplaySize.y);
			ImGuizmo::Manipulate(glm::value_ptr(cameraSceneItem.getCameraRelativeWorldSpaceToViewSpaceMatrix()), glm::value_ptr(cameraSceneItem.getViewSpaceToClipSpaceMatrix(static_cast<float>(imGuiIO.DisplaySize.x) / imGuiIO.DisplaySize.y)), operation, mode, glm::value_ptr(matrix), nullptr, gizmoSettings.useSnap ? &gizmoSettings.snap[0] : nullptr);
			transform = Transform(matrix);
			transform.position += cameraSceneItem.getWorldSpaceCameraPosition();	// Camera relative rendering
		}
	}

	void DebugGuiHelper::drawSkeleton(const CameraSceneItem& cameraSceneItem, const SkeletonMeshSceneItem& skeletonMeshSceneItem)
	{
		// Get skeleton resource instance
		const SkeletonResource* skeletonResource = skeletonMeshSceneItem.getSceneResource().getRenderer().getSkeletonResourceManager().tryGetById(skeletonMeshSceneItem.getSkeletonResourceId());
		if (nullptr != skeletonResource)
		{
			// Get transform data
			glm::mat4 objectSpaceToWorldSpace;
			{
				Transform transform = skeletonMeshSceneItem.getParentSceneNodeSafe().getGlobalTransform();
				transform.position -= cameraSceneItem.getWorldSpaceCameraPosition();	// Camera relative rendering
				transform.getAsMatrix(objectSpaceToWorldSpace);
			}
			const ImGuiIO& imGuiIO = ImGui::GetIO();
			const glm::mat4 objectSpaceToClipSpaceMatrix = cameraSceneItem.getViewSpaceToClipSpaceMatrix(static_cast<float>(imGuiIO.DisplaySize.x) / imGuiIO.DisplaySize.y) * cameraSceneItem.getCameraRelativeWorldSpaceToViewSpaceMatrix() * objectSpaceToWorldSpace;

			// Get skeleton data
			const uint8_t numberOfBones = skeletonResource->getNumberOfBones();
			const uint8_t* boneParentIndices = skeletonResource->getBoneParentIndices();
			const glm::mat4* globalBoneMatrices = skeletonResource->getGlobalBoneMatrices();

			// Draw skeleton hierarchy as lines
			// -> Update ImGui style to not have a visible round border
			ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
			if (ImGui::Begin("skeleton", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
			{
				static const ImColor WHITE_COLOR(255, 255, 255);
				ImDrawList* imDrawList = ImGui::GetWindowDrawList();
				ImVec2 parentBonePosition;
				ImVec2 bonePosition;
				for (uint8_t boneIndex = 1; boneIndex < numberOfBones; ++boneIndex)
				{
					::detail::draw3DLine(objectSpaceToClipSpaceMatrix, globalBoneMatrices[boneParentIndices[boneIndex]][3], globalBoneMatrices[boneIndex][3], WHITE_COLOR, 6.0f, *imDrawList);
				}
			}
			ImGui::End();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}
	}

	void DebugGuiHelper::drawGrid(const CameraSceneItem& cameraSceneItem, float cellSize, double yPosition)
	{
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_FirstUseEver);
		if (ImGui::Begin("grid", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
		{
			const int32_t NUMBER_OF_LINES_PER_DIRECTION = 10;
			static const ImColor GREY_COLOR(0.5f, 0.5f, 0.5f, 1.0f);
			ImDrawList* imDrawList = ImGui::GetWindowDrawList();
			const ImGuiIO& imGuiIO = ImGui::GetIO();
			const glm::mat4 objectSpaceToClipSpaceMatrix = cameraSceneItem.getViewSpaceToClipSpaceMatrix(static_cast<float>(imGuiIO.DisplaySize.x) / imGuiIO.DisplaySize.y) * cameraSceneItem.getCameraRelativeWorldSpaceToViewSpaceMatrix();

			// Keep the grid fixed at the 64 bit world space position of the camera and take camera relative rendering into account
			const glm::dvec3& cameraPosition = cameraSceneItem.getParentSceneNodeSafe().getTransform().position;
			const glm::dvec3& worldSpaceCameraPosition = cameraSceneItem.getWorldSpaceCameraPosition();
			const glm::vec3 centerPosition(Math::makeMultipleOf(cameraPosition.x, cellSize) - worldSpaceCameraPosition.x, yPosition - worldSpaceCameraPosition.y, Math::makeMultipleOf(cameraPosition.z, cellSize) - worldSpaceCameraPosition.z);

			// Lines along z axis
			for (int32_t z = -NUMBER_OF_LINES_PER_DIRECTION; z <= NUMBER_OF_LINES_PER_DIRECTION; ++z)
			{
				const float thickness = (0 == z || NUMBER_OF_LINES_PER_DIRECTION == std::abs(z)) ? 4.0f : 1.0f;
				::detail::draw3DLine(objectSpaceToClipSpaceMatrix, centerPosition + glm::vec3(-NUMBER_OF_LINES_PER_DIRECTION * cellSize, 0.0f, static_cast<float>(z) * cellSize), centerPosition + glm::vec3(NUMBER_OF_LINES_PER_DIRECTION * cellSize, 0.0f, static_cast<float>(z) * cellSize), GREY_COLOR, thickness, *imDrawList);
			}

			// Lines along x axis
			for (int32_t x = -NUMBER_OF_LINES_PER_DIRECTION; x <= NUMBER_OF_LINES_PER_DIRECTION; ++x)
			{
				const float thickness = (0 == x || NUMBER_OF_LINES_PER_DIRECTION == std::abs(x)) ? 4.0f : 1.0f;
				::detail::draw3DLine(objectSpaceToClipSpaceMatrix, centerPosition + glm::vec3(static_cast<float>(x) * cellSize, 0.0f, -NUMBER_OF_LINES_PER_DIRECTION * cellSize), centerPosition + glm::vec3(static_cast<float>(x) * cellSize, 0.0f, NUMBER_OF_LINES_PER_DIRECTION * cellSize), GREY_COLOR, thickness, *imDrawList);
			}
		}
		ImGui::End();
	}


	//[-------------------------------------------------------]
	//[ Private static methods                                ]
	//[-------------------------------------------------------]
	void DebugGuiHelper::drawMetricsWindow(bool& open, CompositorWorkspaceInstance* compositorWorkspaceInstance)
	{
		if (ImGui::Begin("Metrics", &open))
		{
			// Frames per second (FPS)
			const float framesPerSecond = ImGui::GetIO().Framerate;
			ImVec4 color = ::detail::GREEN_COLOR;
			if (framesPerSecond < 60.0f)
			{
				color = ::detail::RED_COLOR;
			}
			else if (framesPerSecond < 90.0f)
			{
				// HTC Vive refresh rate: 90 Hz (11.11 ms per frame), everything below isn't OK
				color = ::detail::YELLOW_COLOR;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / framesPerSecond, framesPerSecond);
			ImGui::PopStyleColor();

			// Optional compositor workspace instance metrics
			if (nullptr != compositorWorkspaceInstance)
			{
				char temporary[128] = {};

				// Please note that one renderable manager can be inside multiple render queue index ranges
				// -> Since this metrics debugging code isn't performance critical we're using already available data to extract the
				//    information we want to display instead of letting the core system gather additional data it doesn't need to work
				size_t numberOfRenderables = 0;
				std::unordered_set<const RenderableManager*> processedRenderableManager;
				for (const CompositorWorkspaceInstance::RenderQueueIndexRange& renderQueueIndexRanges : compositorWorkspaceInstance->getRenderQueueIndexRanges())
				{
					for (const RenderableManager* renderableManager : renderQueueIndexRanges.renderableManagers)
					{
						if (processedRenderableManager.find(renderableManager) == processedRenderableManager.cend())
						{
							processedRenderableManager.insert(renderableManager);
							numberOfRenderables += renderableManager->getRenderables().size();
						}
					}
				}
				ImGui::Text("Rendered renderable managers %s", ::detail::stringFormatCommas(static_cast<uint64_t>(processedRenderableManager.size()), temporary));
				ImGui::Text("Rendered renderables %s", ::detail::stringFormatCommas(static_cast<uint64_t>(numberOfRenderables), temporary));

				// Command buffer metrics
				const Rhi::CommandBuffer& commandBuffer = compositorWorkspaceInstance->getCommandBuffer();
				#ifdef RHI_STATISTICS
					const uint32_t numberOfCommands = commandBuffer.getNumberOfCommands();
				#else
					uint32_t numberOfCommands = 0;
					{
						const uint8_t* commandPacketBuffer = commandBuffer.getCommandPacketBuffer();
						Rhi::ConstCommandPacket constCommandPacket = commandPacketBuffer;
						while (nullptr != constCommandPacket)
						{
							// Count command packet
							++numberOfCommands;

							{ // Next command
								const uint32_t nextCommandPacketByteIndex = Rhi::CommandPacketHelper::getNextCommandPacketByteIndex(constCommandPacket);
								constCommandPacket = (~0u != nextCommandPacketByteIndex) ? &commandPacketBuffer[nextCommandPacketByteIndex] : nullptr;
							}
						}
					}
				#endif
				if (ImGui::TreeNode("EmittedCommands", "Emitted commands: %s", ::detail::stringFormatCommas(numberOfCommands, temporary)))
				{
					// Loop through all commands and count them
					uint32_t numberOfCommandFunctions[static_cast<uint8_t>(Rhi::CommandDispatchFunctionIndex::NUMBER_OF_FUNCTIONS)] = {};
					const uint8_t* commandPacketBuffer = commandBuffer.getCommandPacketBuffer();
					Rhi::ConstCommandPacket constCommandPacket = commandPacketBuffer;
					while (nullptr != constCommandPacket)
					{
						// Count command packet
						++numberOfCommandFunctions[static_cast<uint32_t>(Rhi::CommandPacketHelper::loadCommandDispatchFunctionIndex(constCommandPacket))];

						{ // Next command
							const uint32_t nextCommandPacketByteIndex = Rhi::CommandPacketHelper::getNextCommandPacketByteIndex(constCommandPacket);
							constCommandPacket = (~0u != nextCommandPacketByteIndex) ? &commandPacketBuffer[nextCommandPacketByteIndex] : nullptr;
						}
					}

					// Print the number of emitted command functions
					static constexpr const char* commandFunction[static_cast<uint32_t>(Rhi::CommandDispatchFunctionIndex::NUMBER_OF_FUNCTIONS)] =
					{
						// Command buffer
						"DispatchCommandBuffer",
						// Graphics
						"SetGraphicsRootSignature",
						"SetGraphicsPipelineState",
						"SetGraphicsResourceGroup",
						"SetGraphicsVertexArray",
						"SetGraphicsViewports",
						"SetGraphicsScissorRectangles",
						"SetGraphicsRenderTarget",
						"ClearGraphics",
						"DrawGraphics",
						"DrawIndexedGraphics",
						"DrawMeshTasks",
						// Compute
						"SetComputeRootSignature",
						"SetComputePipelineState",
						"SetComputeResourceGroup",
						"DispatchCompute",
						// Resource
						"SetTextureMinimumMaximumMipmapIndex",
						"ResolveMultisampleFramebuffer",
						"CopyResource",
						"GenerateMipmaps",
						"CopyUniformBufferData",
						"SetUniform",
						// Query
						"ResetQueryPool",
						"BeginQuery",
						"EndQuery",
						"WriteTimestampQuery",
						// Debug
						"SetDebugMarker",
						"BeginDebugEvent",
						"EndDebugEvent"
					};
					for (uint32_t i = 0; i < static_cast<uint32_t>(Rhi::CommandDispatchFunctionIndex::NUMBER_OF_FUNCTIONS); ++i)
					{
						ImGui::Text("%s: %s", commandFunction[i], ::detail::stringFormatCommas(numberOfCommandFunctions[i], temporary));
					}
					ImGui::TreePop();
				}

				// RHI and pipeline statistics
				#ifdef RHI_STATISTICS
				{ // RHI statistics
					const Rhi::Statistics& statistics = static_cast<const Rhi::IRhi&>(compositorWorkspaceInstance->getRenderer().getRhi()).getStatistics();
					if (ImGui::TreeNode("RhiResources", "RHI resources: %s", ::detail::stringFormatCommas(statistics.getNumberOfCurrentResources(), temporary)))
					{
						ImGui::Text("Root signatures: %s", ::detail::stringFormatCommas(statistics.currentNumberOfRootSignatures.load(), temporary));
						ImGui::Text("Resource groups: %s", ::detail::stringFormatCommas(statistics.currentNumberOfResourceGroups.load(), temporary));
						ImGui::Text("Graphics programs: %s", ::detail::stringFormatCommas(statistics.currentNumberOfGraphicsPrograms.load(), temporary));
						ImGui::Text("Vertex arrays: %s", ::detail::stringFormatCommas(statistics.currentNumberOfVertexArrays.load(), temporary));
						ImGui::Text("Render passes: %s", ::detail::stringFormatCommas(statistics.currentNumberOfRenderPasses.load(), temporary));
						ImGui::Text("Query pools: %s", ::detail::stringFormatCommas(statistics.currentNumberOfQueryPools.load(), temporary));
						ImGui::Text("Swap chains: %s", ::detail::stringFormatCommas(statistics.currentNumberOfSwapChains.load(), temporary));
						ImGui::Text("Framebuffers: %s", ::detail::stringFormatCommas(statistics.currentNumberOfFramebuffers.load(), temporary));
						ImGui::Text("Vertex buffers: %s", ::detail::stringFormatCommas(statistics.currentNumberOfVertexBuffers.load(), temporary));
						ImGui::Text("Index buffers: %s", ::detail::stringFormatCommas(statistics.currentNumberOfIndexBuffers.load(), temporary));
						ImGui::Text("Texture buffers: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTextureBuffers.load(), temporary));
						ImGui::Text("Structured buffers: %s", ::detail::stringFormatCommas(statistics.currentNumberOfStructuredBuffers.load(), temporary));
						ImGui::Text("Indirect buffers: %s", ::detail::stringFormatCommas(statistics.currentNumberOfIndirectBuffers.load(), temporary));
						ImGui::Text("Uniform buffers: %s", ::detail::stringFormatCommas(statistics.currentNumberOfUniformBuffers.load(), temporary));
						ImGui::Text("1D textures: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTexture1Ds.load(), temporary));
						ImGui::Text("1D texture arrays: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTexture1DArrays.load(), temporary));
						ImGui::Text("2D textures: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTexture2Ds.load(), temporary));
						ImGui::Text("2D texture arrays: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTexture2DArrays.load(), temporary));
						ImGui::Text("3D textures: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTexture3Ds.load(), temporary));
						ImGui::Text("Cube textures: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTextureCubes.load(), temporary));
						ImGui::Text("Cube texture arrays: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTextureCubeArrays.load(), temporary));
						ImGui::Text("Graphics pipeline states: %s", ::detail::stringFormatCommas(statistics.currentNumberOfGraphicsPipelineStates.load(), temporary));
						ImGui::Text("Compute pipeline states: %s", ::detail::stringFormatCommas(statistics.currentNumberOfComputePipelineStates.load(), temporary));
						ImGui::Text("Sampler states: %s", ::detail::stringFormatCommas(statistics.currentNumberOfSamplerStates.load(), temporary));
						ImGui::Text("Vertex shaders: %s", ::detail::stringFormatCommas(statistics.currentNumberOfVertexShaders.load(), temporary));
						ImGui::Text("Tessellation control shaders: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTessellationControlShaders.load(), temporary));
						ImGui::Text("Tessellation evaluation shaders: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTessellationEvaluationShaders.load(), temporary));
						ImGui::Text("Geometry shaders: %s", ::detail::stringFormatCommas(statistics.currentNumberOfGeometryShaders.load(), temporary));
						ImGui::Text("Fragment shaders: %s", ::detail::stringFormatCommas(statistics.currentNumberOfFragmentShaders.load(), temporary));
						ImGui::Text("Task shaders: %s", ::detail::stringFormatCommas(statistics.currentNumberOfTaskShaders.load(), temporary));
						ImGui::Text("Mesh shaders: %s", ::detail::stringFormatCommas(statistics.currentNumberOfMeshShaders.load(), temporary));
						ImGui::Text("Compute shaders: %s", ::detail::stringFormatCommas(statistics.currentNumberOfComputeShaders.load(), temporary));
						ImGui::TreePop();
					}
				}

				// Pipeline statistics
				if (ImGui::TreeNode("PipelineStatistics", "Pipeline statistics"))
				{
					const Rhi::PipelineStatisticsQueryResult& pipelineStatisticsQueryResult = compositorWorkspaceInstance->getPipelineStatisticsQueryResult();
					ImGui::Text("Input assembler vertices: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfInputAssemblerVertices, temporary));
					ImGui::Text("Input assembler primitives: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfInputAssemblerPrimitives, temporary));
					ImGui::Text("Vertex shader invocations: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfVertexShaderInvocations, temporary));
					ImGui::Text("Geometry shader invocations: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfGeometryShaderInvocations, temporary));
					ImGui::Text("Geometry shader output primitives: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfGeometryShaderOutputPrimitives, temporary));
					ImGui::Text("Clipping input primitives: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfClippingInputPrimitives, temporary));
					ImGui::Text("Clipping output primitives: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfClippingOutputPrimitives, temporary));
					ImGui::Text("Fragment shader invocations: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfFragmentShaderInvocations, temporary));
					ImGui::Text("Tessellation control shader invocations: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfTessellationControlShaderInvocations, temporary));
					ImGui::Text("Tessellation evaluation shader invocations: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfTessellationEvaluationShaderInvocations, temporary));
					ImGui::Text("Compute shader invocations: %s", ::detail::stringFormatCommas(pipelineStatisticsQueryResult.numberOfComputeShaderInvocations, temporary));
					ImGui::TreePop();
				}
				#endif
			}
		}
		ImGui::End();
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
