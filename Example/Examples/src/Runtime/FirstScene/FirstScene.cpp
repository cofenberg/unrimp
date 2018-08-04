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
#include "Runtime/FirstScene/FirstScene.h"
#include "Runtime/FirstScene/FreeCameraController.h"
#ifdef RENDERER_RUNTIME_OPENVR
	#include "Runtime/FirstScene/VrController.h"
#endif
#ifdef _WIN32
	#include <RendererRuntime/Core/Platform/WindowsHeader.h>
#endif

#ifdef RENDERER_TOOLKIT
	#include <RendererToolkit/Public/RendererToolkit.h>
#endif

#include <RendererRuntime/IRendererRuntime.h>
#include <RendererRuntime/Core/Math/EulerAngles.h>
#include <RendererRuntime/Core/Time/TimeManager.h>
#ifdef RENDERER_RUNTIME_IMGUI
	#include <RendererRuntime/DebugGui/ImGuiLog.h>
	#include <RendererRuntime/DebugGui/DebugGuiManager.h>
#endif
#ifdef RENDERER_RUNTIME_OPENVR
	#include <RendererRuntime/Vr/IVrManager.h>
#endif
#include <RendererRuntime/Resource/Scene/SceneNode.h>
#include <RendererRuntime/Resource/Scene/SceneResource.h>
#include <RendererRuntime/Resource/Scene/SceneResourceManager.h>
#include <RendererRuntime/Resource/Scene/Item/Camera/CameraSceneItem.h>
#include <RendererRuntime/Resource/Scene/Item/Light/SunlightSceneItem.h>
#include <RendererRuntime/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.h>
#include <RendererRuntime/Resource/Mesh/MeshResourceManager.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/ICompositorInstancePass.h>
#include <RendererRuntime/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h>
#include <RendererRuntime/Resource/CompositorNode/Pass/DebugGui/CompositorResourcePassDebugGui.h>
#include <RendererRuntime/Resource/MaterialBlueprint/Cache/PipelineStateCompiler.h>
#include <RendererRuntime/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h>
#include <RendererRuntime/Resource/Material/MaterialResourceManager.h>
#include <RendererRuntime/Resource/Material/MaterialResource.h>
#include <RendererRuntime/Resource/Texture/TextureResourceManager.h>
#include <RendererRuntime/Resource/Detail/ResourceStreamer.h>
#include <RendererRuntime/Context.h>

#include <Renderer/DefaultAllocator.h>

#include <DeviceInput/DeviceInput.h>

#ifdef RENDERER_RUNTIME_IMGUI
	#include <imgui/imgui.h>
#endif

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/gtc/type_ptr.hpp>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Global variables                                      ]
//[-------------------------------------------------------]
extern Renderer::DefaultAllocator g_DefaultAllocator;


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
		static constexpr uint32_t SCENE_ASSET_ID		  = STRING_ID("Example/Scene/Default/FirstScene");
		static constexpr uint32_t IMROD_MATERIAL_ASSET_ID = STRING_ID("Example/Material/Character/Imrod");


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
FirstScene::FirstScene() :
	mInputManager(new DeviceInput::InputManager()),
	mImGuiLog(nullptr),
	mCompositorWorkspaceInstance(nullptr),
	mSceneResourceId(RendererRuntime::getInvalid<RendererRuntime::SceneResourceId>()),
	mMaterialResourceId(RendererRuntime::getInvalid<RendererRuntime::MaterialResourceId>()),
	mCloneMaterialResourceId(RendererRuntime::getInvalid<RendererRuntime::MaterialResourceId>()),
	mCustomMaterialResourceSet(false),
	mController(nullptr),
	// Crazy raw-pointers to point-of-interest scene stuff
	mCameraSceneItem(nullptr),
	mSunlightSceneItem(nullptr),
	mSkeletonMeshSceneItem(nullptr),
	mSceneNode(nullptr),
	// Video
	mFullscreen(false),
	mCurrentFullscreen(false),
	mResolutionScale(1.0f),
	mUseVerticalSynchronization(false),
	mCurrentUseVerticalSynchronization(false),
	mCurrentMsaa(Msaa::FOUR),
	// Graphics
	mInstancedCompositor(Compositor::FORWARD),
	mCurrentCompositor(mInstancedCompositor),
	mHighQualityLighting(true),
	mSoftParticles(true),
	mCurrentTextureFiltering(TextureFiltering::ANISOTROPIC_4),
	mNumberOfTopTextureMipmapsToRemove(0),
	mTerrainTessellatedTriangleWidth(16),
	// Environment
	mCloudsIntensity(1.0f),
	mWindSpeed(0.01f),
	mWetSurfaces{0.0f, 0.6f, 0.4f, 1.0f},
	// Post processing
	mPerformFxaa(false),
	mPerformSharpen(true),
	mPerformChromaticAberration(false),
	mPerformOldCrtEffect(false),
	mPerformFilmGrain(false),
	mPerformSepiaColorCorrection(false),
	mPerformVignette(false),
	mDepthOfFieldBlurrinessCutoff(0.0f),
	// Selected material properties
	mUseEmissiveMap(true),
	mAlbedoColor{1.0f, 1.0f, 1.0f},
	// Selected scene item
	mRotationSpeed(0.5f),
	mShowSkeleton(false),
	// Scene hot-reloading memory
	mHasCameraTransformBackup(false)
{
	#ifdef RENDERER_RUNTIME_IMGUI
		RendererRuntime::DebugGuiManager::setImGuiAllocatorFunctions(g_DefaultAllocator);
		mImGuiLog = new RendererRuntime::ImGuiLog();
		setCustomLog(mImGuiLog);
	#endif
}

FirstScene::~FirstScene()
{
	// The resources are released within "onDeinitialization()"

	// Destroy our input manager instance
	delete mInputManager;

	// Destroy our ImGui log instance
	#ifdef RENDERER_RUNTIME_IMGUI
		delete mImGuiLog;
	#endif
}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstScene::onInitialization()
{
	// Get and check the renderer runtime instance
	RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		// Usability: Restore the position and size of the main window from a previous session
		#if defined(_WIN32) && !defined(SDL2_FOUND) && defined(RENDERER_RUNTIME_IMGUI)
		{
			float value[4] = {};
			if (rendererRuntime->getDebugGuiManager().getIniSetting("MainWindowPositionSize", value))
			{
				::SetWindowPos(reinterpret_cast<HWND>(rendererRuntime->getRenderer().getContext().getNativeWindowHandle()), HWND_TOP, static_cast<int>(value[0]), static_cast<int>(value[1]), static_cast<int>(value[2]), static_cast<int>(value[3]), 0);
			}
		}
		#endif

		// TODO(co) Remove this after the Vulkan renderer backend is fully up-and-running
		if (rendererRuntime->getRenderer().getNameId() == Renderer::NameId::VULKAN)
		{
			mCurrentCompositor = mInstancedCompositor = Compositor::DEBUG;
			rendererRuntime->getMaterialBlueprintResourceManager().setCreateInitialPipelineStateCaches(false);
		}

		// Create the scene resource
		rendererRuntime->getSceneResourceManager().loadSceneResourceByAssetId(::detail::SCENE_ASSET_ID, mSceneResourceId, this);

		// Load the material resource we're going to clone
		rendererRuntime->getMaterialResourceManager().loadMaterialResourceByAssetId(::detail::IMROD_MATERIAL_ASSET_ID, mMaterialResourceId, this);

		// Try to startup the VR-manager if a HMD is present
		#ifdef RENDERER_RUNTIME_OPENVR
		{
			RendererRuntime::IVrManager& vrManager = rendererRuntime->getVrManager();
			if (vrManager.isHmdPresent())
			{
				vrManager.setSceneResourceId(mSceneResourceId);
				if (vrManager.startup(STRING_ID("Example/Material/Default/VrDevice")))
				{
					// Select the VR compositor and enable MSAA by default since image stability is quite important for VR
					// -> "Advanced VR Rendering" by Alex Vlachos, Valve -> page 26 -> "4xMSAA Minimum Quality" ( http://media.steampowered.com/apps/valve/2015/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf )
					if (Compositor::DEBUG != mCurrentCompositor)
					{
						mCurrentCompositor = mInstancedCompositor = Compositor::VR;
					}
					mCurrentMsaa = Msaa::FOUR;
					mCurrentTextureFiltering = TextureFiltering::ANISOTROPIC_4;
				}
			}
		}
		#endif

		// When using OpenGL ES 3, switch to a compositor which is designed for mobile devices
		// TODO(co) The Vulkan renderer backend is under construction, so debug compositor for now
		if (rendererRuntime->getRenderer().getNameId() == Renderer::NameId::VULKAN || rendererRuntime->getRenderer().getNameId() == Renderer::NameId::OPENGLES3)
		{
			// TODO(co) Add compositor designed for mobile devices, for now we're using the most simple debug compositor to have something on the screen
			mCurrentCompositor = mInstancedCompositor = Compositor::DEBUG;
			mCurrentMsaa = Msaa::NONE;
			mCurrentTextureFiltering = TextureFiltering::BILINEAR;
		}

		// Create the compositor workspace instance
		createCompositorWorkspace();
	}
}

void FirstScene::onDeinitialization()
{
	// Release the used resources
	delete mCompositorWorkspaceInstance;
	mCompositorWorkspaceInstance = nullptr;
	RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		rendererRuntime->getSceneResourceManager().destroySceneResource(mSceneResourceId);
		RendererRuntime::setInvalid(mSceneResourceId);
	}

	// Destroy controller instance
	if (nullptr != mController)
	{
		delete mController;
		mController = nullptr;
	}
}

void FirstScene::onUpdate()
{
	const RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		{ // Tell the material blueprint resource manager about our global material properties
			RendererRuntime::MaterialProperties& globalMaterialProperties = rendererRuntime->getMaterialBlueprintResourceManager().getGlobalMaterialProperties();
			// Graphics
			globalMaterialProperties.setPropertyById(STRING_ID("GlobalHighQualityLighting"), RendererRuntime::MaterialPropertyValue::fromBoolean(mHighQualityLighting));
			globalMaterialProperties.setPropertyById(STRING_ID("GlobalSoftParticles"), RendererRuntime::MaterialPropertyValue::fromBoolean(mSoftParticles));
			globalMaterialProperties.setPropertyById(STRING_ID("GlobalTessellatedTriangleWidth"), RendererRuntime::MaterialPropertyValue::fromFloat(static_cast<float>(mTerrainTessellatedTriangleWidth)));
			// Environment
			globalMaterialProperties.setPropertyById(STRING_ID("GlobalCloudsIntensity"), RendererRuntime::MaterialPropertyValue::fromFloat(mCloudsIntensity));
			globalMaterialProperties.setPropertyById(STRING_ID("GlobalWindDirectionStrength"), RendererRuntime::MaterialPropertyValue::fromFloat4(1.0f, 0.0f, 0.0f, mWindSpeed));
			globalMaterialProperties.setPropertyById(STRING_ID("GlobalUseWetSurfaces"), RendererRuntime::MaterialPropertyValue::fromBoolean(mWetSurfaces[0] > 0.0f));
			globalMaterialProperties.setPropertyById(STRING_ID("GlobalWetSurfaces"), RendererRuntime::MaterialPropertyValue::fromFloat4(mWetSurfaces));
		}

		// Update the scene node rotation
		if (nullptr != mSceneNode && mRotationSpeed > 0.0f)
		{
			glm::vec3 eulerAngles = RendererRuntime::EulerAngles::matrixToEuler(glm::mat3_cast(mSceneNode->getGlobalTransform().rotation));
			eulerAngles.x += rendererRuntime->getTimeManager().getPastSecondsSinceLastFrame() * mRotationSpeed;
			mSceneNode->setRotation(RendererRuntime::EulerAngles::eulerToQuaternion(eulerAngles));
		}

		// Update controller
		if (nullptr != mController)
		{
			// Simple GUI <-> ingame input distribution
			// -> Do only enable input as long as this example application has the operation system window focus
			// -> While the mouse is hovering over an GUI element, disable the ingame controller
			// -> Avoid that while looking around with the mouse the mouse is becoming considered hovering over an GUI element
			// -> Remember: Unrimp is about rendering related topics, it's not an all-in-one-framework including an advanced input framework, so a simple non-generic solution is sufficient in here
			#ifdef _WIN32
				const bool hasWindowFocus = (::GetFocus() == reinterpret_cast<HWND>(rendererRuntime->getRenderer().getContext().getNativeWindowHandle()));
			#else
				bool hasWindowFocus = true;
			#endif
			#ifdef RENDERER_RUNTIME_IMGUI
				const bool isAnyWindowHovered = ImGui::IsAnyWindowHovered();
			#else
				const bool isAnyWindowHovered = false;
			#endif
			mController->onUpdate(rendererRuntime->getTimeManager().getPastSecondsSinceLastFrame(), hasWindowFocus && (mController->isMouseControlInProgress() || !isAnyWindowHovered));
		}

		// Scene hot-reloading memory
		if (nullptr != mCameraSceneItem)
		{
			mHasCameraTransformBackup = true;
			mCameraTransformBackup = mCameraSceneItem->getParentSceneNodeSafe().getGlobalTransform();

			// Backup camera position and rotation for a following session, but only if VR isn't running right now
			#ifdef RENDERER_RUNTIME_IMGUI
				#ifdef RENDERER_RUNTIME_OPENVR
					if (!rendererRuntime->getVrManager().isRunning())
				#endif
				{
					RendererRuntime::DebugGuiManager& debugGuiManager = mCompositorWorkspaceInstance->getRendererRuntime().getDebugGuiManager();
					{
						const float value[4] = { mCameraTransformBackup.position.x, mCameraTransformBackup.position.y, mCameraTransformBackup.position.z, 0.0f };
						debugGuiManager.setIniSetting("CameraPosition", value);
					}
					debugGuiManager.setIniSetting("CameraRotation", glm::value_ptr(mCameraTransformBackup.rotation));
				}
			#endif
		}

		// Usability: Backup the position and size of the main window so we can restore it in the next session
		#if defined(_WIN32) && defined(RENDERER_RUNTIME_IMGUI)
		{
			RECT rect;
			::GetWindowRect(reinterpret_cast<HWND>(rendererRuntime->getRenderer().getContext().getNativeWindowHandle()), &rect);
			const float value[4] = { static_cast<float>(rect.left), static_cast<float>(rect.top), static_cast<float>(rect.right - rect.left), static_cast<float>(rect.bottom - rect.top) };
			rendererRuntime->getDebugGuiManager().setIniSetting("MainWindowPositionSize", value);
		}
		#endif
	}

	// TODO(co) We need to get informed when the mesh scene item received the mesh resource loading finished signal
	trySetCustomMaterialResource();

	// Update the input system
	mInputManager->Update();
}

void FirstScene::onDraw()
{
	Renderer::IRenderTarget* mainRenderTarget = getMainRenderTarget();
	RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != mainRenderTarget && nullptr != rendererRuntime && nullptr != mCompositorWorkspaceInstance)
	{
		applyCurrentSettings(*mainRenderTarget);
		createDebugGui(*mainRenderTarget);
		RendererRuntime::SceneResource* sceneResource = rendererRuntime->getSceneResourceManager().tryGetById(mSceneResourceId);
		if (nullptr != sceneResource && sceneResource->getLoadingState() == RendererRuntime::IResource::LoadingState::LOADED)
		{
			// Execute the compositor workspace instance
			mCompositorWorkspaceInstance->executeVr(*mainRenderTarget, mCameraSceneItem, mSunlightSceneItem);
		}
	}
}


//[-------------------------------------------------------]
//[ Protected virtual RendererRuntime::IResourceListener methods ]
//[-------------------------------------------------------]
void FirstScene::onLoadingStateChange(const RendererRuntime::IResource& resource)
{
	const RendererRuntime::IResource::LoadingState loadingState = resource.getLoadingState();
	if (resource.getAssetId() == ::detail::SCENE_ASSET_ID)
	{
		if (RendererRuntime::IResource::LoadingState::LOADED == loadingState)
		{
			// Sanity checks
			assert(nullptr == mSceneNode);
			assert(nullptr == mCameraSceneItem);
			assert(nullptr == mSunlightSceneItem);
			assert(nullptr == mSkeletonMeshSceneItem);

			// Loop through all scene nodes and grab the first found camera, directional light and mesh
			const RendererRuntime::SceneResource& sceneResource = static_cast<const RendererRuntime::SceneResource&>(resource);
			for (RendererRuntime::SceneNode* sceneNode : sceneResource.getSceneNodes())
			{
				// Loop through all scene items attached to the current scene node
				for (RendererRuntime::ISceneItem* sceneItem : sceneNode->getAttachedSceneItems())
				{
					switch (sceneItem->getSceneItemTypeId())
					{
						case RendererRuntime::MeshSceneItem::TYPE_ID:
							// Grab the first found mesh scene item scene node
							if (nullptr == mSceneNode)
							{
								mSceneNode = sceneNode;
								trySetCustomMaterialResource();
							}
							break;

						case RendererRuntime::CameraSceneItem::TYPE_ID:
							// Grab the first found camera scene item
							if (nullptr == mCameraSceneItem)
							{
								mCameraSceneItem = static_cast<RendererRuntime::CameraSceneItem*>(sceneItem);
								if (mHasCameraTransformBackup)
								{
									// Scene hot-reloading memory
									mCameraSceneItem->getParentSceneNodeSafe().setTransform(mCameraTransformBackup);
								}
							}
							break;

						case RendererRuntime::SunlightSceneItem::TYPE_ID:
							// Grab the first found sunlight scene item
							if (nullptr == mSunlightSceneItem)
							{
								mSunlightSceneItem = static_cast<RendererRuntime::SunlightSceneItem*>(sceneItem);
							}
							break;

						case RendererRuntime::SkeletonMeshSceneItem::TYPE_ID:
							// Grab the first found skeleton mesh scene item
							if (nullptr == mSkeletonMeshSceneItem)
							{
								mSkeletonMeshSceneItem = static_cast<RendererRuntime::SkeletonMeshSceneItem*>(sceneItem);
							}
							break;
					}
				}
			}

			if (nullptr != mCameraSceneItem && nullptr != mCameraSceneItem->getParentSceneNode())
			{
				#ifdef RENDERER_RUNTIME_OPENVR
					if (mCompositorWorkspaceInstance->getRendererRuntime().getVrManager().isRunning())
					{
						mController = new VrController(*mCameraSceneItem);

						// For VR, set camera to origin
						RendererRuntime::SceneNode* sceneNode = mCameraSceneItem->getParentSceneNode();
						sceneNode->setPosition(RendererRuntime::Math::VEC3_ZERO);
						sceneNode->setRotation(RendererRuntime::Math::QUAT_IDENTITY);
					}
					else
				#endif
				{
					mController = new FreeCameraController(*mInputManager, *mCameraSceneItem);

					// Restore camera position and rotation from a previous session if virtual reality is disabled
					#ifdef RENDERER_RUNTIME_IMGUI
						if (!mHasCameraTransformBackup)
						{
							float value[4] = {};
							RendererRuntime::DebugGuiManager& debugGuiManager = mCompositorWorkspaceInstance->getRendererRuntime().getDebugGuiManager();
							if (debugGuiManager.getIniSetting("CameraPosition", value))
							{
								mCameraSceneItem->getParentSceneNode()->setPosition(glm::vec3(value[0], value[1], value[2]));
							}
							if (debugGuiManager.getIniSetting("CameraRotation", value))
							{
								mCameraSceneItem->getParentSceneNode()->setRotation(glm::quat(value[3], value[0], value[1], value[2]));
							}
						}
					#endif
				}
			}
		}
		else
		{
			mCameraSceneItem = nullptr;
			mSunlightSceneItem = nullptr;
			mSkeletonMeshSceneItem = nullptr;
			if (nullptr != mController)
			{
				delete mController;
				mController = nullptr;
			}
			mSceneNode = nullptr;
		}
	}
	else if (RendererRuntime::IResource::LoadingState::LOADED == loadingState && resource.getAssetId() == ::detail::IMROD_MATERIAL_ASSET_ID)
	{
		// Create our material resource clone
		RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
		if (nullptr != rendererRuntime)
		{
			mCloneMaterialResourceId = rendererRuntime->getMaterialResourceManager().createMaterialResourceByCloning(resource.getId());
			trySetCustomMaterialResource();
		}
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void FirstScene::applyCurrentSettings(Renderer::IRenderTarget& mainRenderTarget)
{
	RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != mCompositorWorkspaceInstance && RendererRuntime::isValid(mSceneResourceId) && nullptr != rendererRuntime)
	{
		// Changes in main swap chain?
		if (mCurrentFullscreen != mFullscreen)
		{
			mCurrentFullscreen = mFullscreen;
			static_cast<Renderer::ISwapChain&>(mainRenderTarget).setFullscreenState(mCurrentFullscreen);
		}
		if (mCurrentUseVerticalSynchronization != mUseVerticalSynchronization)
		{
			mCurrentUseVerticalSynchronization = mUseVerticalSynchronization;
			static_cast<Renderer::ISwapChain&>(mainRenderTarget).setVerticalSynchronizationInterval(mCurrentUseVerticalSynchronization ? 1u : 0u);
		}

		// Recreate the compositor workspace instance, if required
		if (mInstancedCompositor != mCurrentCompositor)
		{
			mInstancedCompositor = static_cast<Compositor>(mCurrentCompositor);
			createCompositorWorkspace();
		}

		// Update texture related settings
		{ // Default texture filtering
			RendererRuntime::MaterialBlueprintResourceManager& materialBlueprintResourceManager = rendererRuntime->getMaterialBlueprintResourceManager();
			switch (mCurrentTextureFiltering)
			{
				case TextureFiltering::POINT:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Renderer::FilterMode::MIN_MAG_MIP_POINT, 1);
					break;

				case TextureFiltering::BILINEAR:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Renderer::FilterMode::MIN_MAG_LINEAR_MIP_POINT, 1);
					break;

				case TextureFiltering::TRILINEAR:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Renderer::FilterMode::MIN_MAG_MIP_LINEAR, 1);
					break;

				case TextureFiltering::ANISOTROPIC_2:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Renderer::FilterMode::ANISOTROPIC, 2);
					break;

				case TextureFiltering::ANISOTROPIC_4:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Renderer::FilterMode::ANISOTROPIC, 4);
					break;

				case TextureFiltering::ANISOTROPIC_8:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Renderer::FilterMode::ANISOTROPIC, 8);
					break;

				case TextureFiltering::ANISOTROPIC_16:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Renderer::FilterMode::ANISOTROPIC, 16);
					break;
			}
		}
		rendererRuntime->getTextureResourceManager().setNumberOfTopMipmapsToRemove(static_cast<uint8_t>(mNumberOfTopTextureMipmapsToRemove));

		// Update compositor workspace
		{ // MSAA
			static constexpr uint8_t NUMBER_OF_MULTISAMPLES[4] = { 1, 2, 4, 8 };
			uint8_t numberOfMultisamples = NUMBER_OF_MULTISAMPLES[mCurrentMsaa];
			const uint8_t maximumNumberOfMultisamples = rendererRuntime->getRenderer().getCapabilities().maximumNumberOfMultisamples;
			if (numberOfMultisamples > maximumNumberOfMultisamples)
			{
				numberOfMultisamples = maximumNumberOfMultisamples;
			}
			mCompositorWorkspaceInstance->setNumberOfMultisamples(numberOfMultisamples);
		}
		mCompositorWorkspaceInstance->setResolutionScale(mResolutionScale);

		{ // Update the material resource instance
			const RendererRuntime::MaterialResourceManager& materialResourceManager = rendererRuntime->getMaterialResourceManager();

			// Depth of field compositor material
			RendererRuntime::MaterialResource* materialResource = materialResourceManager.getMaterialResourceByAssetId(STRING_ID("Example/MaterialBlueprint/Compositor/DepthOfField"));
			if (nullptr != materialResource)
			{
				materialResource->setPropertyById(STRING_ID("BlurrinessCutoff"), RendererRuntime::MaterialPropertyValue::fromFloat(mDepthOfFieldBlurrinessCutoff));
			}

			// Final compositor material
			materialResource = materialResourceManager.getMaterialResourceByAssetId(STRING_ID("Example/MaterialBlueprint/Compositor/Final"));
			if (nullptr != materialResource)
			{
				static constexpr uint32_t IDENTITY_TEXTURE_ASSET_ID = STRING_ID("Unrimp/Texture/DynamicByCode/IdentityColorCorrectionLookupTable3D");
				static constexpr uint32_t SEPIA_TEXTURE_ASSET_ID = STRING_ID("Example/Texture/Compositor/SepiaColorCorrectionLookupTable16x1");
				materialResource->setPropertyById(STRING_ID("ColorCorrectionLookupTableMap"), RendererRuntime::MaterialPropertyValue::fromTextureAssetId(mPerformSepiaColorCorrection ? SEPIA_TEXTURE_ASSET_ID : IDENTITY_TEXTURE_ASSET_ID));
				materialResource->setPropertyById(STRING_ID("Fxaa"), RendererRuntime::MaterialPropertyValue::fromBoolean(mPerformFxaa));
				materialResource->setPropertyById(STRING_ID("Sharpen"), RendererRuntime::MaterialPropertyValue::fromBoolean(mPerformSharpen));
				materialResource->setPropertyById(STRING_ID("ChromaticAberration"), RendererRuntime::MaterialPropertyValue::fromBoolean(mPerformChromaticAberration));
				materialResource->setPropertyById(STRING_ID("OldCrtEffect"), RendererRuntime::MaterialPropertyValue::fromBoolean(mPerformOldCrtEffect));
				materialResource->setPropertyById(STRING_ID("FilmGrain"), RendererRuntime::MaterialPropertyValue::fromBoolean(mPerformFilmGrain));
				materialResource->setPropertyById(STRING_ID("Vignette"), RendererRuntime::MaterialPropertyValue::fromBoolean(mPerformVignette));
			}

			// Imrod material clone
			materialResource = materialResourceManager.tryGetById(mCloneMaterialResourceId);
			if (nullptr != materialResource)
			{
				materialResource->setPropertyById(STRING_ID("UseEmissiveMap"), RendererRuntime::MaterialPropertyValue::fromBoolean(mUseEmissiveMap));
				materialResource->setPropertyById(STRING_ID("AlbedoColor"), RendererRuntime::MaterialPropertyValue::fromFloat3(mAlbedoColor));
			}
		}
	}
}

void FirstScene::createCompositorWorkspace()
{
	RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
	if (nullptr != rendererRuntime)
	{
		// Create/recreate the compositor workspace instance
		static constexpr uint32_t COMPOSITOR_WORKSPACE_ASSET_ID[4] = {
			STRING_ID("Example/CompositorWorkspace/Default/Debug"),
			STRING_ID("Example/CompositorWorkspace/Default/Forward"),
			STRING_ID("Example/CompositorWorkspace/Default/Deferred"),
			STRING_ID("Example/CompositorWorkspace/Default/Vr")
		};
		delete mCompositorWorkspaceInstance;
		mCompositorWorkspaceInstance = new RendererRuntime::CompositorWorkspaceInstance(*rendererRuntime, COMPOSITOR_WORKSPACE_ASSET_ID[mInstancedCompositor]);
	}
}

void FirstScene::createDebugGui(MAYBE_UNUSED Renderer::IRenderTarget& mainRenderTarget)
{
	#ifdef RENDERER_RUNTIME_IMGUI
		RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
		if (nullptr != mCompositorWorkspaceInstance && RendererRuntime::isValid(mSceneResourceId) && nullptr != rendererRuntime)
		{
			// Get the render target the debug GUI is rendered into, use the provided main render target as fallback
			const RendererRuntime::ICompositorInstancePass* compositorInstancePass = mCompositorWorkspaceInstance->getFirstCompositorInstancePassByCompositorPassTypeId(RendererRuntime::CompositorResourcePassDebugGui::TYPE_ID);
			if (nullptr != compositorInstancePass)
			{
				// Setup GUI
				RendererRuntime::DebugGuiManager& debugGuiManager = rendererRuntime->getDebugGuiManager();
				debugGuiManager.newFrame(((nullptr != compositorInstancePass->getRenderTarget()) ? *compositorInstancePass->getRenderTarget() : mainRenderTarget), mCompositorWorkspaceInstance);
				mImGuiLog->draw(rendererRuntime->getContext().getFileManager());
				if (ImGui::Begin("Options"))
				{
					// Status
					static const ImVec4 GREY_COLOR(0.5f, 0.5f, 0.5f, 1.0f);
					static const ImVec4 RED_COLOR(1.0f, 0.0f, 0.0f, 1.0f);
					ImGui::PushStyleColor(ImGuiCol_Text, GREY_COLOR);
						ImGui::Text("Renderer: %s", mainRenderTarget.getRenderer().getName());
						ImGui::Text("GPU: %s", mainRenderTarget.getRenderer().getCapabilities().deviceName);
						#ifdef RENDERER_TOOLKIT
						{
							const RendererToolkit::IRendererToolkit* rendererToolkit = getRendererToolkit();
							if (nullptr != rendererToolkit)
							{
								const bool idle = (RendererToolkit::IRendererToolkit::State::IDLE == rendererToolkit->getState());
								ImGui::PushStyleColor(ImGuiCol_Text, idle ? GREY_COLOR : RED_COLOR);
									ImGui::Text("Renderer Toolkit: %s", idle ? "Idle" : "Busy");
								ImGui::PopStyleColor();
							}
						}
						#endif
						{ // Resource streamer
							const bool idle = (0 == rendererRuntime->getResourceStreamer().getNumberOfInFlightLoadRequests());
							ImGui::PushStyleColor(ImGuiCol_Text, idle ? GREY_COLOR : RED_COLOR);
								ImGui::Text("Resource Streamer: %s", idle ? "Idle" : "Busy");
							ImGui::PopStyleColor();
						}
						ImGui::Text("Pipeline State Compiler: %s", (0 == rendererRuntime->getPipelineStateCompiler().getNumberOfInFlightCompilerRequests()) ? "Idle" : "Busy");
					ImGui::PopStyleColor();
					if (ImGui::Button("Log"))
					{
						mImGuiLog->open();
					}
					ImGui::SameLine();
					if (ImGui::Button("Metrics"))
					{
						debugGuiManager.openMetricsWindow();
					}
					ImGui::Separator();

					// Video
					if (ImGui::BeginMenu("Video"))
					{
						// TODO(co) Add fullscreen combo box (window, borderless window, native fullscreen)
						mFullscreen = static_cast<Renderer::ISwapChain&>(mainRenderTarget).getFullscreenState();	// It's possible to toggle fullscreen by using ALT-return, take this into account
						ImGui::Checkbox("Fullscreen", &mFullscreen);
						// TODO(co) Add resolution and refresh rate combo box
						ImGui::SliderFloat("Resolution Scale", &mResolutionScale, 0.05f, 4.0f, "%.3f");
						ImGui::Checkbox("Vertical Synchronization", &mUseVerticalSynchronization);
						if (rendererRuntime->getRenderer().getCapabilities().maximumNumberOfMultisamples > 1)
						{
							static constexpr const char* items[] = { "None", "2x", "4x", "8x" };
							ImGui::Combo("MSAA", &mCurrentMsaa, items, static_cast<int>(glm::countof(items)));
						}
						ImGui::EndMenu();
					}

					// Graphics
					if (ImGui::BeginMenu("Graphics"))
					{
						{
							static constexpr const char* items[] = { "Debug", "Forward", "Deferred", "VR" };
							ImGui::Combo("Compositor", &mCurrentCompositor, items, static_cast<int>(glm::countof(items)));
						}
						ImGui::Checkbox("High Quality Lighting", &mHighQualityLighting);
						ImGui::Checkbox("Soft-Particles", &mSoftParticles);
						{
							static constexpr const char* items[] = { "Point", "Bilinear", "Trilinear", "2x Anisotropic", "4x Anisotropic", "8x Anisotropic", "16x Anisotropic" };
							ImGui::Combo("Texture filtering", &mCurrentTextureFiltering, items, static_cast<int>(glm::countof(items)));
						}
						ImGui::SliderInt("Mipmaps to Remove", &mNumberOfTopTextureMipmapsToRemove, 0, 8);
						ImGui::SliderInt("Terrain Tessellated Triangle Width", &mTerrainTessellatedTriangleWidth, 0, 64);
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("Desired pixels per triangle edge, lower value increases tessellation and hence decreases the performance");
						}
						ImGui::EndMenu();
					}

					// Environment
					if (ImGui::BeginMenu("Environment"))
					{
						if (nullptr != mSunlightSceneItem)
						{
							float timeOfDay = mSunlightSceneItem->getTimeOfDay();
							ImGui::SliderFloat("Time of Day", &timeOfDay, 0.0f, 23.59f, "%.2f");
							mSunlightSceneItem->setTimeOfDay(timeOfDay);
						}
						ImGui::SliderFloat("Clouds Intensity", &mCloudsIntensity, 0.0f, 10.0f, "%.3f");
						ImGui::SliderFloat("Wind Speed", &mWindSpeed, 0.0f, 1.0f, "%.3f");
						ImGui::SliderFloat("Wetness", &mWetSurfaces[0], 0.0f, 1.0f, "%.3f");
						ImGui::EndMenu();
					}

					// Post processing
					if (ImGui::BeginMenu("Post Processing"))
					{
						{ // Mutually exclusive
							int activeRadioButton = -1;
							if (mPerformFxaa)
							{
								activeRadioButton = 0;
							}
							else if (mPerformSharpen)
							{
								activeRadioButton = 1;
							}
							else if (mPerformChromaticAberration)
							{
								activeRadioButton = 2;
							}
							else if (mPerformOldCrtEffect)
							{
								activeRadioButton = 3;
							}
							ImGui::RadioButton("-",					   &activeRadioButton, -1);
							ImGui::RadioButton("FXAA",				   &activeRadioButton, 0);
							ImGui::RadioButton("Sharpen",			   &activeRadioButton, 1);
							ImGui::RadioButton("Chromatic Aberration", &activeRadioButton, 2);
							ImGui::RadioButton("Old CRT",			   &activeRadioButton, 3);
							ImGui::Separator();
							mPerformFxaa				= (0 == activeRadioButton);
							mPerformSharpen				= (1 == activeRadioButton);
							mPerformChromaticAberration	= (2 == activeRadioButton);
							mPerformOldCrtEffect		= (3 == activeRadioButton);
						}
						ImGui::Checkbox("Film Grain", &mPerformFilmGrain);
						ImGui::Checkbox("Sepia Color Correction", &mPerformSepiaColorCorrection);
						ImGui::Checkbox("Vignette", &mPerformVignette);
						ImGui::SliderFloat("Depth of Field", &mDepthOfFieldBlurrinessCutoff, 0.0f, 1.0f, "%.3f");
						ImGui::EndMenu();
					}

					// Selected material properties
					if (ImGui::BeginMenu("Selected Material"))
					{
						ImGui::Checkbox("Use Emissive Map", &mUseEmissiveMap);
						ImGui::ColorEdit3("Albedo Color", mAlbedoColor);
						ImGui::EndMenu();
					}

					// Selected scene item
					if (ImGui::BeginMenu("Selected Scene Item"))
					{
						ImGui::SliderFloat("Rotation Speed", &mRotationSpeed, 0.0f, 2.0f, "%.3f");
						ImGui::Checkbox("Show Skeleton", &mShowSkeleton);
						ImGui::EndMenu();
					}
					if (nullptr != mCameraSceneItem)
					{
						// Draw skeleton
						if (mShowSkeleton && nullptr != mSkeletonMeshSceneItem && nullptr != mSkeletonMeshSceneItem->getParentSceneNode())
						{
							RendererRuntime::DebugGuiHelper::drawSkeleton(*mCameraSceneItem, *mSkeletonMeshSceneItem);
						}

						// Scene node transform using gizmo
						if (nullptr != mSceneNode)
						{
							// Draw gizmo
							ImGui::Separator();
							RendererRuntime::Transform transform = mSceneNode->getGlobalTransform();
							RendererRuntime::DebugGuiHelper::drawGizmo(*mCameraSceneItem, mGizmoSettings, transform);
							mSceneNode->setTransform(transform);

							// Draw grid
							// TODO(co) Make this optional via GUI
							// RendererRuntime::DebugGuiHelper::drawGrid(*mCameraSceneItem, transform.position.y);
						}
					}
				}
				ImGui::End();
			}
		}
	#endif
}

void FirstScene::trySetCustomMaterialResource()
{
	if (!mCustomMaterialResourceSet && nullptr != mSceneNode && RendererRuntime::isValid(mCloneMaterialResourceId))
	{
		const RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
		if (nullptr != rendererRuntime)
		{
			for (RendererRuntime::ISceneItem* sceneItem : mSceneNode->getAttachedSceneItems())
			{
				if (sceneItem->getSceneItemTypeId() == RendererRuntime::MeshSceneItem::TYPE_ID)
				{
					// Tell the mesh scene item about our custom material resource
					RendererRuntime::MeshSceneItem* meshSceneItem = static_cast<RendererRuntime::MeshSceneItem*>(sceneItem);
					if (RendererRuntime::IResource::LoadingState::LOADED == rendererRuntime->getMeshResourceManager().getResourceByResourceId(meshSceneItem->getMeshResourceId()).getLoadingState())
					{
						meshSceneItem->setMaterialResourceIdOfAllSubMeshes(mCloneMaterialResourceId);
						mCustomMaterialResourceSet = true;
					}
				}
			}
		}
	}
}
