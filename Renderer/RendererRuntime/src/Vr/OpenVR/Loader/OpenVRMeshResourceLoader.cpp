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
#include "RendererRuntime/Vr/OpenVR/Loader/OpenVRMeshResourceLoader.h"
#include "RendererRuntime/Vr/OpenVR/Loader/OpenVRTextureResourceLoader.h"
#include "RendererRuntime/Resource/Mesh/MeshResource.h"
#include "RendererRuntime/Resource/Mesh/MeshResourceManager.h"
#include "RendererRuntime/Resource/Texture/TextureResourceManager.h"
#include "RendererRuntime/Resource/Material/MaterialResourceManager.h"
#include "RendererRuntime/Resource/Material/MaterialResource.h"
#include "RendererRuntime/Vr/OpenVR/VrManagerOpenVR.h"
#include "RendererRuntime/Core/Math/Math.h"
#include "RendererRuntime/IRendererRuntime.h"
#include "RendererRuntime/Context.h"

#include <openvr/openvr.h>

#include <mikktspace/mikktspace.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::_Func_impl_no_alloc<Concurrency::details::_Task_impl<_ReturnType>::_CancelAndRunContinuations::<lambda_0456396a71e3abd88ede77bdd2823d8e>,_Ret>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::_Func_impl_no_alloc<Concurrency::details::_Task_impl<_ReturnType>::_CancelAndRunContinuations::<lambda_0456396a71e3abd88ede77bdd2823d8e>,_Ret>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Func_impl_no_alloc<Concurrency::details::_Task_impl<_ReturnType>::_CancelAndRunContinuations::<lambda_0456396a71e3abd88ede77bdd2823d8e>,_Ret>': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Func_impl_no_alloc<Concurrency::details::_Task_impl<_ReturnType>::_CancelAndRunContinuations::<lambda_0456396a71e3abd88ede77bdd2823d8e>,_Ret>': move assignment operator was implicitly defined as deleted
	#include <chrono>
	#include <thread>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{

	namespace mikktspace
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr uint32_t NUMBER_OF_VERTICES_PER_FACE = 3;


		//[-------------------------------------------------------]
		//[ Global variables                                      ]
		//[-------------------------------------------------------]
		SMikkTSpaceContext g_MikkTSpaceContext;


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		int getNumFaces(const SMikkTSpaceContext* pContext)
		{
			const RendererRuntime::OpenVRMeshResourceLoader* openVRMeshResourceLoader = static_cast<const RendererRuntime::OpenVRMeshResourceLoader*>(pContext->m_pUserData);
			return static_cast<int>(openVRMeshResourceLoader->getVrRenderModel()->unTriangleCount);
		}

		int getNumVerticesOfFace(const SMikkTSpaceContext*, const int)
		{
			return NUMBER_OF_VERTICES_PER_FACE;
		}

		void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
		{
			const RendererRuntime::OpenVRMeshResourceLoader* openVRMeshResourceLoader = static_cast<const RendererRuntime::OpenVRMeshResourceLoader*>(pContext->m_pUserData);
			const vr::RenderModel_t* vrRenderModel = openVRMeshResourceLoader->getVrRenderModel();
			const vr::HmdVector3_t& position = vrRenderModel->rVertexData[vrRenderModel->rIndexData[iFace * NUMBER_OF_VERTICES_PER_FACE + iVert]].vPosition;
			fvPosOut[0] = position.v[0];
			fvPosOut[1] = position.v[1];
			fvPosOut[2] = position.v[2];
		}

		void getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
		{
			const RendererRuntime::OpenVRMeshResourceLoader* openVRMeshResourceLoader = static_cast<const RendererRuntime::OpenVRMeshResourceLoader*>(pContext->m_pUserData);
			const vr::RenderModel_t* vrRenderModel = openVRMeshResourceLoader->getVrRenderModel();
			const vr::HmdVector3_t& normal = vrRenderModel->rVertexData[vrRenderModel->rIndexData[iFace * NUMBER_OF_VERTICES_PER_FACE + iVert]].vNormal;
			fvNormOut[0] = normal.v[0];
			fvNormOut[1] = normal.v[1];
			fvNormOut[2] = normal.v[2];
		}

		void getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
		{
			const RendererRuntime::OpenVRMeshResourceLoader* openVRMeshResourceLoader = static_cast<const RendererRuntime::OpenVRMeshResourceLoader*>(pContext->m_pUserData);
			const vr::RenderModel_t* vrRenderModel = openVRMeshResourceLoader->getVrRenderModel();
			const float* texCoord = vrRenderModel->rVertexData[vrRenderModel->rIndexData[iFace * NUMBER_OF_VERTICES_PER_FACE + iVert]].rfTextureCoord;
			fvTexcOut[0] = texCoord[0];
			fvTexcOut[1] = texCoord[1];
		}

		void setTSpace(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fvBiTangent[], const float, const float, const tbool, const int iFace, const int iVert)
		{
			RendererRuntime::OpenVRMeshResourceLoader* openVRMeshResourceLoader = static_cast<RendererRuntime::OpenVRMeshResourceLoader*>(pContext->m_pUserData);
			const size_t index = openVRMeshResourceLoader->getVrRenderModel()->rIndexData[iFace * NUMBER_OF_VERTICES_PER_FACE + iVert];

			{ // Tangent
				glm::vec3& tangent = openVRMeshResourceLoader->getTangentsData()[index];
				tangent.x = fvTangent[0];
				tangent.y = fvTangent[1];
				tangent.z = fvTangent[2];
			}

			{ // Binormal
				glm::vec3& binormal = openVRMeshResourceLoader->getBinormalsData()[index];
				binormal.x = fvBiTangent[0];
				binormal.y = fvBiTangent[1];
				binormal.z = fvBiTangent[2];
			}
		}


	} // mikktspace

	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr uint32_t NUMBER_OF_BYTES_PER_VERTEX = sizeof(float) * 3 + sizeof(float) * 2 + sizeof(short) * 4;


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		RendererRuntime::AssetId setupRenderModelAlbedoTexture(const RendererRuntime::IRendererRuntime& rendererRuntime, const vr::RenderModel_t& vrRenderModel)
		{
			// Check whether or not we need to generate the runtime mesh asset right now
			RendererRuntime::AssetId assetId = RendererRuntime::VrManagerOpenVR::albedoTextureIdToAssetId(vrRenderModel.diffuseTextureId);
			RendererRuntime::TextureResourceId textureResourceId = RendererRuntime::getInvalid<RendererRuntime::TextureResourceId>();
			const bool rgbHardwareGammaCorrection = true;	// TODO(co) It must be possible to set the property name from the outside: Ask the material blueprint whether or not hardware gamma correction should be used
			rendererRuntime.getTextureResourceManager().loadTextureResourceByAssetId(assetId, STRING_ID("Unrimp/Texture/DynamicByCode/IdentityAlbedoMap2D"), textureResourceId, nullptr, rgbHardwareGammaCorrection, false, RendererRuntime::OpenVRTextureResourceLoader::TYPE_ID);

			// Done
			return assetId;
		}

		RendererRuntime::MaterialResourceId setupRenderModelMaterial(const RendererRuntime::IRendererRuntime& rendererRuntime, RendererRuntime::MaterialResourceId vrDeviceMaterialResourceId, vr::TextureID_t vrTextureId, RendererRuntime::AssetId albedoTextureAssetId)
		{
			// Get the texture name and convert it into an runtime material asset ID
			const std::string materialName = "OpenVR_" + std::to_string(vrTextureId);
			RendererRuntime::AssetId materialAssetId = RendererRuntime::StringId(materialName.c_str());

			// Check whether or not we need to generate the runtime material asset right now
			RendererRuntime::MaterialResourceManager& materialResourceManager = rendererRuntime.getMaterialResourceManager();
			RendererRuntime::MaterialResourceId materialResourceId = materialResourceManager.getMaterialResourceIdByAssetId(materialAssetId);
			if (RendererRuntime::isInvalid(materialResourceId))
			{
				// We need to generate the runtime material asset right now
				materialResourceId = materialResourceManager.createMaterialResourceByCloning(vrDeviceMaterialResourceId, materialAssetId);
				if (RendererRuntime::isValid(materialResourceId))
				{
					RendererRuntime::MaterialResource* materialResource = materialResourceManager.tryGetById(materialResourceId);
					if (nullptr != materialResource)
					{
						// TODO(co) It must be possible to set the property name from the outside
						materialResource->setPropertyById(STRING_ID("_argb_nxa"), RendererRuntime::MaterialPropertyValue::fromTextureAssetId(albedoTextureAssetId));
					}
				}
			}

			// Done
			return materialResourceId;
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IResourceLoader methods ]
	//[-------------------------------------------------------]
	void OpenVRMeshResourceLoader::onProcessing()
	{
		// Load the render model
		const std::string& renderModelName = getRenderModelName();
		vr::IVRRenderModels* vrRenderModels = vr::VRRenderModels();
		vr::EVRRenderModelError vrRenderModelError = vr::VRRenderModelError_Loading;
		while (vrRenderModelError == vr::VRRenderModelError_Loading)
		{
			vrRenderModelError = vrRenderModels->LoadRenderModel_Async(renderModelName.c_str(), &mVrRenderModel);
			if (vrRenderModelError == vr::VRRenderModelError_Loading)
			{
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(1ms);
			}
		}
		if (vr::VRRenderModelError_None != vrRenderModelError)
		{
			RENDERER_LOG(mRendererRuntime.getContext(), CRITICAL, "The renderer runtime was unable to load OpenVR render model \"%s\": %s", renderModelName.c_str(), vrRenderModels->GetRenderModelErrorNameFromEnum(vrRenderModelError))
			return;
		}

		// Setup "mikktspace" by Morten S. Mikkelsen for semi-standard tangent space generation (see e.g. https://wiki.blender.org/index.php/Dev:Shading/Tangent_Space_Normal_Maps for background information)
		SMikkTSpaceInterface mikkTSpaceInterface;
		mikkTSpaceInterface.m_getNumFaces		   = mikktspace::getNumFaces;
		mikkTSpaceInterface.m_getNumVerticesOfFace = mikktspace::getNumVerticesOfFace;
		mikkTSpaceInterface.m_getPosition		   = mikktspace::getPosition;
		mikkTSpaceInterface.m_getNormal			   = mikktspace::getNormal;
		mikkTSpaceInterface.m_getTexCoord		   = mikktspace::getTexCoord;
		mikkTSpaceInterface.m_setTSpaceBasic	   = nullptr;
		mikkTSpaceInterface.m_setTSpace			   = mikktspace::setTSpace;
		mikktspace::g_MikkTSpaceContext.m_pInterface = &mikkTSpaceInterface;
		mikktspace::g_MikkTSpaceContext.m_pUserData  = this;

		{ // Get the vertex buffer and index buffer data
			mMinimumBoundingBoxPosition = glm::vec3(std::numeric_limits<float>::max());
			mMaximumBoundingBoxPosition = glm::vec3(std::numeric_limits<float>::lowest());

			// Tell the mesh resource about the number of vertices and indices
			const uint32_t numberOfVertices = mVrRenderModel->unVertexCount;
			const uint32_t numberOfIndices = mVrRenderModel->unTriangleCount * 3;
			mMeshResource->setNumberOfVertices(numberOfVertices);
			mMeshResource->setNumberOfIndices(numberOfIndices);

			{ // Fill the vertex buffer data
				const uint32_t numberOfBytes = numberOfVertices * ::detail::NUMBER_OF_BYTES_PER_VERTEX;
				mVertexBufferData.resize(numberOfBytes);
				uint8_t* currentVertexBufferData = mVertexBufferData.data();
				const vr::RenderModel_Vertex_t* currentVrRenderModelVertex = mVrRenderModel->rVertexData;
				mTangentsData.resize(mVrRenderModel->unVertexCount);
				mBinormalsData.resize(mVrRenderModel->unVertexCount);
				if (genTangSpaceDefault(&mikktspace::g_MikkTSpaceContext) == 0)
				{
					// TODO(co) Error handling
					assert(false && "mikktspace for semi-standard tangent space generation failed");
				}
				for (uint32_t i = 0; i < numberOfVertices; ++i, ++currentVrRenderModelVertex)
				{
					const float* vrRenderModelVertexPosition = currentVrRenderModelVertex->vPosition.v;

					{ // Update minimum and maximum bounding box position
						const glm::vec3 glmVertex(vrRenderModelVertexPosition[0], vrRenderModelVertexPosition[1], -vrRenderModelVertexPosition[2]);
						mMinimumBoundingBoxPosition = glm::min(mMinimumBoundingBoxPosition, glmVertex);
						mMaximumBoundingBoxPosition = glm::max(mMaximumBoundingBoxPosition, glmVertex);
					}

					{ // 32 bit position
						float* position = reinterpret_cast<float*>(currentVertexBufferData);
						position[0] = vrRenderModelVertexPosition[0];
						position[1] = vrRenderModelVertexPosition[1];
						position[2] = -vrRenderModelVertexPosition[2];
						currentVertexBufferData += sizeof(float) * 3;
					}

					{ // 32 bit texture coordinate
						float* textureCoordinate = reinterpret_cast<float*>(currentVertexBufferData);
						textureCoordinate[0] = currentVrRenderModelVertex->rfTextureCoord[0];
						textureCoordinate[1] = currentVrRenderModelVertex->rfTextureCoord[1];
						currentVertexBufferData += sizeof(float) * 2;
					}

					{ // 16 bit QTangent
						// Get the mesh vertex normal, tangent and binormal
						const glm::vec3 normal(currentVrRenderModelVertex->vNormal.v[0], currentVrRenderModelVertex->vNormal.v[1], currentVrRenderModelVertex->vNormal.v[2]);
						const glm::vec3& tangent = mTangentsData[i];
						const glm::vec3& binormal = mBinormalsData[i];

						// Generate tangent frame rotation matrix
						glm::mat3 tangentFrame(
							tangent.x,  tangent.y,  tangent.z,
							binormal.x, binormal.y, binormal.z,
							normal.x,   normal.y,   normal.z
						);

						// Calculate tangent frame quaternion
						const glm::quat tangentFrameQuaternion = Math::calculateTangentFrameQuaternion(tangentFrame);

						// Set our vertex buffer 16 bit QTangent
						short* qTangent = reinterpret_cast<short*>(currentVertexBufferData);
						qTangent[0] = static_cast<short>(tangentFrameQuaternion.x * SHRT_MAX);
						qTangent[1] = static_cast<short>(tangentFrameQuaternion.y * SHRT_MAX);
						qTangent[2] = static_cast<short>(tangentFrameQuaternion.z * SHRT_MAX);
						qTangent[3] = static_cast<short>(tangentFrameQuaternion.w * SHRT_MAX);
						currentVertexBufferData += sizeof(short) * 4;
					}
				}
			}

			{ // Fill the index buffer data
			  // -> We need to flip the vertex winding so we don't need to modify rasterizer states
				mIndexBufferData.resize(numberOfIndices);
				uint16_t* currentIndexBufferData = mIndexBufferData.data();
				for (uint32_t i = 0; i < mVrRenderModel->unTriangleCount; ++i, currentIndexBufferData += 3)
				{
					const uint32_t offset = i * 3;
					currentIndexBufferData[0] = mVrRenderModel->rIndexData[offset + 2];
					currentIndexBufferData[1] = mVrRenderModel->rIndexData[offset + 1];
					currentIndexBufferData[2] = mVrRenderModel->rIndexData[offset + 0];
				}
			}
		}

		// Can we create the renderer resource asynchronous as well?
		if (mRendererRuntime.getRenderer().getCapabilities().nativeMultiThreading)
		{
			mVertexArray = createVertexArray();
		}
	}

	bool OpenVRMeshResourceLoader::onDispatch()
	{
		// Bounding
		// -> Calculate the bounding sphere radius enclosing the bounding box (don't use the inner bounding box radius)
		mMeshResource->setBoundingBoxPosition(mMinimumBoundingBoxPosition, mMaximumBoundingBoxPosition);
		mMeshResource->setBoundingSpherePositionRadius((mMinimumBoundingBoxPosition + mMaximumBoundingBoxPosition) * 0.5f, Math::calculateInnerBoundingSphereRadius(mMinimumBoundingBoxPosition, mMaximumBoundingBoxPosition));

		// Create vertex array object (VAO)
		mMeshResource->setVertexArray(mRendererRuntime.getRenderer().getCapabilities().nativeMultiThreading ? mVertexArray : createVertexArray());

		{ // Create sub-meshes
			// Load the render model texture and setup the material asset
			// -> We don't care if loading of the albedo texture fails in here, isn't that important and the show must go on
			const AssetId albedoTextureAssetId = ::detail::setupRenderModelAlbedoTexture(mRendererRuntime, *mVrRenderModel);
			const MaterialResourceId materialResourceId = ::detail::setupRenderModelMaterial(mRendererRuntime, static_cast<const VrManagerOpenVR&>(mRendererRuntime.getVrManager()).getVrDeviceMaterialResourceId(), mVrRenderModel->diffuseTextureId, albedoTextureAssetId);

			// Tell the mesh resource about the sub-mesh
			mMeshResource->getSubMeshes().push_back(SubMesh(materialResourceId, 0, mMeshResource->getNumberOfIndices()));
		}

		// Free the render model
		if (nullptr != mVrRenderModel)
		{
			vr::VRRenderModels()->FreeRenderModel(mVrRenderModel);
			mVrRenderModel = nullptr;
		}

		// Fully loaded?
		return true;
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	Renderer::IVertexArray* OpenVRMeshResourceLoader::createVertexArray() const
	{
		Renderer::IBufferManager& bufferManager = mRendererRuntime.getBufferManager();

		// Create the vertex buffer
		Renderer::IVertexBuffer* vertexBuffer = bufferManager.createVertexBuffer(static_cast<uint32_t>(mVertexBufferData.size()), mVertexBufferData.data(), 0, Renderer::BufferUsage::STATIC_DRAW);
		RENDERER_SET_RESOURCE_DEBUG_NAME(vertexBuffer, getRenderModelName().c_str())

		// Create the index buffer
		Renderer::IIndexBuffer* indexBuffer = bufferManager.createIndexBuffer(static_cast<uint32_t>(mIndexBufferData.size() * sizeof(uint16_t)), Renderer::IndexBufferFormat::UNSIGNED_SHORT, mIndexBufferData.data(), 0, Renderer::BufferUsage::STATIC_DRAW);
		RENDERER_SET_RESOURCE_DEBUG_NAME(indexBuffer, getRenderModelName().c_str())

		// Create vertex array object (VAO)
		const Renderer::VertexArrayVertexBuffer vertexArrayVertexBuffers[] = { vertexBuffer, mRendererRuntime.getMeshResourceManager().getDrawIdVertexBufferPtr() };
		Renderer::IVertexArray* vertexArray = bufferManager.createVertexArray(MeshResource::VERTEX_ATTRIBUTES, static_cast<uint32_t>(glm::countof(vertexArrayVertexBuffers)), vertexArrayVertexBuffers, indexBuffer);
		RENDERER_SET_RESOURCE_DEBUG_NAME(vertexArray, getRenderModelName().c_str())

		// Done
		return vertexArray;
	}

	const std::string& OpenVRMeshResourceLoader::getRenderModelName() const
	{
		// OpenVR render model names can get awful long due to absolute path information, so, we need to store them inside a separate list and tell the asset just about the render model name index
		const VrManagerOpenVR::RenderModelNames& renderModelNames = static_cast<const VrManagerOpenVR&>(mRendererRuntime.getVrManager()).getRenderModelNames();
		const uint32_t renderModelNameIndex = static_cast<uint32_t>(std::atoi(getAsset().virtualFilename));
		assert(renderModelNameIndex < static_cast<uint32_t>(renderModelNames.size()));
		return renderModelNames[renderModelNameIndex];
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
