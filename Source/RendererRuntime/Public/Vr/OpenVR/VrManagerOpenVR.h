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
#include "RendererRuntime/Public/Vr/IVrManager.h"
#include "RendererRuntime/Public/Resource/IResourceListener.h"

#include <Renderer/Public/Renderer.h>

#include <OpenVR/openvr.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class SceneNode;
	class IRendererRuntime;
	class OpenVRRuntimeLinking;
	class IVrManagerOpenVRListener;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t MaterialResourceId;	///< POD material resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	class VrManagerOpenVR final : public IVrManager, public IResourceListener
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RendererRuntimeImpl;


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("VrManagerOpenVR");
		typedef std::vector<std::string> RenderModelNames;


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] static AssetId albedoTextureIdToAssetId(vr::TextureID_t albedoTextureId);


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline IVrManagerOpenVRListener& getVrManagerOpenVRListener() const
		{
			// We know this pointer must always be valid
			RENDERER_ASSERT(mRendererRuntime.getContext(), nullptr != mVrManagerOpenVRListener, "Invalid VR manager OpenVR listener")
			return *mVrManagerOpenVRListener;
		}

		RENDERERRUNTIME_API_EXPORT void setVrManagerOpenVRListener(IVrManagerOpenVRListener* vrManagerOpenVRListener);	// Does not take over the control of the memory

		[[nodiscard]] inline vr::IVRSystem* getVrSystem() const
		{
			return mVrSystem;
		}

		[[nodiscard]] inline MaterialResourceId getVrDeviceMaterialResourceId() const
		{
			return mVrDeviceMaterialResourceId;
		}

		[[nodiscard]] inline const RenderModelNames& getRenderModelNames() const
		{
			return mRenderModelNames;
		}

		[[nodiscard]] inline const vr::TrackedDevicePose_t& getVrTrackedDevicePose(vr::TrackedDeviceIndex_t trackedDeviceIndex) const
		{
			RENDERER_ASSERT(mRendererRuntime.getContext(), trackedDeviceIndex < vr::k_unMaxTrackedDeviceCount, "Maximum tracked device count exceeded")
			return mVrTrackedDevicePose[trackedDeviceIndex];
		}

		[[nodiscard]] inline const glm::mat4& getDevicePoseMatrix(vr::TrackedDeviceIndex_t trackedDeviceIndex) const
		{
			RENDERER_ASSERT(mRendererRuntime.getContext(), trackedDeviceIndex < vr::k_unMaxTrackedDeviceCount, "Maximum tracked device count exceeded")
			return mDevicePoseMatrix[trackedDeviceIndex];
		}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::IVrManager methods    ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual VrManagerTypeId getVrManagerTypeId() const override;
		[[nodiscard]] virtual bool isHmdPresent() const override;
		virtual void setSceneResourceId(SceneResourceId sceneResourceId) override;
		[[nodiscard]] virtual bool startup(AssetId vrDeviceMaterialAssetId) override;

		[[nodiscard]] inline virtual bool isRunning() const override
		{
			return (nullptr != mVrSystem);
		}

		virtual void shutdown() override;
		virtual void updateHmdMatrixPose(CameraSceneItem* cameraSceneItem) override;
		[[nodiscard]] virtual glm::mat4 getHmdViewSpaceToClipSpaceMatrix(VrEye vrEye, float nearZ, float farZ) const override;
		[[nodiscard]] virtual glm::mat4 getHmdEyeSpaceToHeadSpaceMatrix(VrEye vrEye) const override;

		[[nodiscard]] inline virtual const glm::mat4& getHmdHeadSpaceToWorldSpaceMatrix() const override
		{
			return mHmdHeadSpaceToWorldSpaceMatrix;
		}

		[[nodiscard]] inline virtual const glm::mat4& getPreviousHmdHeadSpaceToWorldSpaceMatrix() const override
		{
			return mPreviousHmdHeadSpaceToWorldSpaceMatrix;
		}


	//[-------------------------------------------------------]
	//[ Private virtual RendererRuntime::IVrManager methods   ]
	//[-------------------------------------------------------]
	private:
		virtual void executeCompositorWorkspaceInstance(CompositorWorkspaceInstance& compositorWorkspaceInstance, Renderer::IRenderTarget& renderTarget, CameraSceneItem* cameraSceneItem, const LightSceneItem* lightSceneItem) override;


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IResourceListener methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void onLoadingStateChange(const IResource& resource) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VrManagerOpenVR(IRendererRuntime& rendererRuntime);
		virtual ~VrManagerOpenVR() override;
		explicit VrManagerOpenVR(const VrManagerOpenVR&) = delete;
		VrManagerOpenVR& operator=(const VrManagerOpenVR&) = delete;
		void setupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex);


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct Component final
		{
			std::string name;
			SceneNode* sceneNode = nullptr;	// TODO(co) No crazy raw-pointers
			Component(const std::string& _name, SceneNode* _sceneNode) :
				name(_name),
				sceneNode(_sceneNode)
			{};
		};
		typedef std::vector<Component> Components;
		struct TrackedDeviceInformation final
		{
			std::string renderModelName;
			Components  components;
		};


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime&		   mRendererRuntime;			///< Renderer runtime instance, do not destroy the instance
		IVrManagerOpenVRListener*  mVrManagerOpenVRListener;	///< OpenVR manager listener, always valid, do not destroy the instance
		bool					   mVrDeviceMaterialResourceLoaded;
		MaterialResourceId		   mVrDeviceMaterialResourceId;
		SceneResourceId			   mSceneResourceId;
		SceneNode*				   mSceneNodes[vr::k_unMaxTrackedDeviceCount];	// TODO(co) No crazy raw-pointers
		TrackedDeviceInformation   mTrackedDeviceInformation[vr::k_unMaxTrackedDeviceCount];
		OpenVRRuntimeLinking*	   mOpenVRRuntimeLinking;
		vr::ETextureType		   mVrTextureType;
		vr::IVRSystem*			   mVrSystem;
		vr::IVRRenderModels*	   mVrRenderModels;
		RenderModelNames		   mRenderModelNames;
		bool					   mShowRenderModels;
		// Transform
		vr::TrackedDevicePose_t	mVrTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		glm::mat4				mDevicePoseMatrix[vr::k_unMaxTrackedDeviceCount];
		uint32_t				mNumberOfValidDevicePoses;
		glm::mat4				mHmdHeadSpaceToWorldSpaceMatrix;
		glm::mat4				mPreviousHmdHeadSpaceToWorldSpaceMatrix;
		// Renderer resources
		Renderer::ITexture2DPtr	  mColorTexture2D;	///< Color 2D texture, can be a null pointer
		Renderer::IFramebufferPtr mFramebuffer;		///< Framebuffer object (FBO), can be a null pointer


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
