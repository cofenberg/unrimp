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
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Examples/Private/Renderer/FirstScene/FirstScene.h"
#include "Examples/Private/Renderer/FirstScene/FreeCameraController.h"
#ifdef RENDERER_OPENVR
	#include "Examples/Private/Renderer/FirstScene/VrController.h"
#endif
#ifdef _WIN32
	#include <Renderer/Public/Core/Platform/WindowsHeader.h>
#endif

#ifdef RENDERER_TOOLKIT
	#include <RendererToolkit/Public/RendererToolkit.h>
#endif

#include <Renderer/Public/IRenderer.h>
#include <Renderer/Public/Core/Math/EulerAngles.h>
#include <Renderer/Public/Core/Time/TimeManager.h>
#ifdef RENDERER_GRAPHICS_DEBUGGER
	#include <Renderer/Public/Core/RenderDocGraphicsDebugger.h>
#endif
#ifdef RENDERER_IMGUI
	#include <Renderer/Public/DebugGui/ImGuiLog.h>
	#include <Renderer/Public/DebugGui/DebugGuiManager.h>
#endif
#ifdef RENDERER_OPENVR
	#include <Renderer/Public/Vr/IVrManager.h>
#endif
#include <Renderer/Public/Resource/Scene/SceneNode.h>
#include <Renderer/Public/Resource/Scene/SceneResource.h>
#include <Renderer/Public/Resource/Scene/SceneResourceManager.h>
#include <Renderer/Public/Resource/Scene/Item/Camera/CameraSceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Light/SunlightSceneItem.h>
#include <Renderer/Public/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.h>
#include <Renderer/Public/Resource/Mesh/MeshResourceManager.h>
#include <Renderer/Public/Resource/CompositorNode/CompositorNodeInstance.h>
#include <Renderer/Public/Resource/CompositorNode/Pass/DebugGui/CompositorResourcePassDebugGui.h>
#include <Renderer/Public/Resource/CompositorNode/Pass/ShadowMap/CompositorInstancePassShadowMap.h>
#include <Renderer/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h>
#include <Renderer/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCompiler.h>
#include <Renderer/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateCompiler.h>
#include <Renderer/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h>
#include <Renderer/Public/Resource/Material/MaterialResourceManager.h>
#include <Renderer/Public/Resource/Material/MaterialResource.h>
#include <Renderer/Public/Resource/Texture/TextureResourceManager.h>
#include <Renderer/Public/Resource/ResourceStreamer.h>
#include <Renderer/Public/Context.h>

#include <Rhi/Public/DefaultAllocator.h>

#include <DeviceInput/DeviceInput.h>

#define INI_IMPLEMENTATION
#define INI_MALLOC(ctx, size) (static_cast<Rhi::IAllocator*>(ctx)->reallocate(nullptr, 0, size, 1))
#define INI_FREE(ctx, ptr) (static_cast<Rhi::IAllocator*>(ctx)->reallocate(ptr, 0, 0, 1))
#include <ini/ini.h>

#ifdef RENDERER_IMGUI
	#include <ImGui/imgui.h>
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
extern Rhi::DefaultAllocator g_DefaultAllocator;


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
		static constexpr char*    VIRTUAL_SETTINGS_FILENAME	= "LocalData/FirstSceneExample.ini";
		static constexpr uint32_t SCENE_ASSET_ID			= ASSET_ID("Example/Scene/S_FirstScene");
		static constexpr uint32_t IMROD_MATERIAL_ASSET_ID	= ASSET_ID("Example/Mesh/Imrod/M_Imrod");


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
	mFirstFrame(true),
	mSceneResourceId(Renderer::getInvalid<Renderer::SceneResourceId>()),
	mMaterialResourceId(Renderer::getInvalid<Renderer::MaterialResourceId>()),
	mCloneMaterialResourceId(Renderer::getInvalid<Renderer::MaterialResourceId>()),
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
	mCurrentMsaa(static_cast<int>(Msaa::TWO)),
	// Graphics
	mInstancedCompositor(Compositor::FORWARD),
	mCurrentCompositor(static_cast<int>(mInstancedCompositor)),
	mShadowQuality(ShadowQuality::HIGH),
	mCurrentShadowQuality(static_cast<int>(mShadowQuality)),
	mHighQualityRendering(true),
	mHighQualityLighting(true),
	mSoftParticles(true),
	mCurrentTextureFiltering(static_cast<int>(TextureFiltering::ANISOTROPIC_4)),
	mNumberOfTopTextureMipmapsToRemove(0),
	mNumberOfTopMeshLodsToRemove(0),
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
	mHasCameraTransformBackup(false),
	// Ini settings indices
	mIni(nullptr),
	mMainWindowPositionSizeIniProperty(INI_NOT_FOUND),
	mCameraPositionRotationIniProperty(INI_NOT_FOUND),
	mOpenMetricsWindowIniProperty(INI_NOT_FOUND)
{
	#ifdef RENDERER_IMGUI
		Renderer::DebugGuiManager::setImGuiAllocatorFunctions(g_DefaultAllocator);
		mImGuiLog = new Renderer::ImGuiLog();
		setCustomLog(mImGuiLog);
	#endif
}

FirstScene::~FirstScene()
{
	// The resources are released within "onDeinitialization()"

	// Destroy our input manager instance
	delete mInputManager;

	// Destroy our ImGui log instance
	#ifdef RENDERER_IMGUI
		delete mImGuiLog;
	#endif
}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void FirstScene::onInitialization()
{
	loadIni();

	// Get and check the renderer instance
	const Renderer::IRenderer& renderer = getRendererSafe();

	// Usability: Restore the position and size of the main window from a previous session
	#if defined(_WIN32) && !defined(SDL2_FOUND) && defined(RENDERER_IMGUI)
		if (INI_NOT_FOUND != mMainWindowPositionSizeIniProperty)
		{
			const char* propertyValue = ini_property_value(mIni, INI_GLOBAL_SECTION, mMainWindowPositionSizeIniProperty);
			if (nullptr != propertyValue)
			{
				int value[4] = { 0, 0, 1024, 768 };
				sscanf(propertyValue, "%d %d %d %d", &value[0], &value[1], &value[2], &value[3]);
				::SetWindowPos(reinterpret_cast<HWND>(renderer.getRhi().getContext().getNativeWindowHandle()), HWND_TOP, static_cast<int>(value[0]), static_cast<int>(value[1]), static_cast<int>(value[2]), static_cast<int>(value[3]), 0);
			}
		}
	#endif

	// Usability: Restore open metrics window
	if (INI_NOT_FOUND != mOpenMetricsWindowIniProperty)
	{
		const char* propertyValue = ini_property_value(mIni, INI_GLOBAL_SECTION, mOpenMetricsWindowIniProperty);
		if (nullptr != propertyValue)
		{
			int value = 0;
			sscanf(propertyValue, "%d", &value);
			if (0 != value)
			{
				renderer.getDebugGuiManager().openMetricsWindow();
			}
		}
	}

	// TODO(co) Remove this after the Vulkan/Direct3D 12 RHI implementation is fully up-and-running. Or better, add asset properties so one can e.g. add asset information regarding e.g. supported RHI implementations.
	const Rhi::NameId nameId = renderer.getRhi().getNameId();
	if (Rhi::NameId::VULKAN == nameId || Rhi::NameId::DIRECT3D12 == nameId || Rhi::NameId::DIRECT3D10 == nameId || Rhi::NameId::DIRECT3D9 == nameId)
	{
		mInstancedCompositor = Compositor::DEBUG;
		mCurrentCompositor = static_cast<int>(mInstancedCompositor);
		if (Rhi::NameId::VULKAN == nameId)
		{
			renderer.getMaterialBlueprintResourceManager().setCreateInitialPipelineStateCaches(false);
		}
	}

	// Create the scene resource
	renderer.getSceneResourceManager().loadSceneResourceByAssetId(::detail::SCENE_ASSET_ID, mSceneResourceId, this);

	// Load the material resource we're going to clone
	renderer.getMaterialResourceManager().loadMaterialResourceByAssetId(::detail::IMROD_MATERIAL_ASSET_ID, mMaterialResourceId, this);

	// Try to startup the VR-manager if a HMD is present
	#ifdef RENDERER_OPENVR
	{
		Renderer::IVrManager& vrManager = renderer.getVrManager();
		if (vrManager.isHmdPresent())
		{
			vrManager.setSceneResourceId(mSceneResourceId);
			if (vrManager.startup(ASSET_ID("Example/Blueprint/Mesh/M_VrDevice")))
			{
				// Select the VR compositor and enable MSAA by default since image stability is quite important for VR
				// -> "Advanced VR Rendering" by Alex Vlachos, Valve -> page 26 -> "4xMSAA Minimum Quality" ( http://media.steampowered.com/apps/valve/2015/Alex_Vlachos_Advanced_VR_Rendering_GDC2015.pdf )
				// -> We're using temporal MSAA which looks quite stable with 2xMSAA as well
				if (Compositor::DEBUG != static_cast<Compositor>(mCurrentCompositor))
				{
					mInstancedCompositor = Compositor::VR;
					mCurrentCompositor = static_cast<int>(mInstancedCompositor);
				}
				if (mCurrentMsaa < static_cast<int>(Msaa::TWO))
				{
					mCurrentMsaa = static_cast<int>(Msaa::TWO);
				}
				if (mCurrentTextureFiltering < static_cast<int>(TextureFiltering::ANISOTROPIC_4))
				{
					mCurrentTextureFiltering = static_cast<int>(TextureFiltering::ANISOTROPIC_4);
				}
			}
		}
	}
	#endif

	// When using OpenGL ES 3, switch to a compositor which is designed for mobile devices
	// TODO(co) The Vulkan/Direct3D 12 RHI implementation is under construction, so debug compositor for now
	if (Rhi::NameId::VULKAN == nameId || Rhi::NameId::DIRECT3D12 == nameId || Rhi::NameId::OPENGLES3 == nameId)
	{
		// TODO(co) Add compositor designed for mobile devices, for now we're using the most simple debug compositor to have something on the screen
		mInstancedCompositor = Compositor::DEBUG;
		mCurrentCompositor = static_cast<int>(mInstancedCompositor);
		mCurrentMsaa = static_cast<int>(Msaa::NONE);
		mCurrentTextureFiltering = static_cast<int>(TextureFiltering::BILINEAR);
	}

	// Create the compositor workspace instance
	createCompositorWorkspace();
}

void FirstScene::onDeinitialization()
{
	saveIni();
	destroyIni();

	// Release the used resources
	delete mCompositorWorkspaceInstance;
	mCompositorWorkspaceInstance = nullptr;
	getRendererSafe().getSceneResourceManager().destroySceneResource(mSceneResourceId);
	Renderer::setInvalid(mSceneResourceId);

	// Destroy controller instance
	if (nullptr != mController)
	{
		delete mController;
		mController = nullptr;
	}
}

void FirstScene::onUpdate()
{
	const Renderer::IRenderer& renderer = getRendererSafe();

	{ // Tell the material blueprint resource manager about our global material properties
		Renderer::MaterialProperties& globalMaterialProperties = renderer.getMaterialBlueprintResourceManager().getGlobalMaterialProperties();
		// Graphics
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalReceiveShadows"), Renderer::MaterialPropertyValue::fromBoolean(ShadowQuality::NONE != mShadowQuality));
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalHighQualityRendering"), Renderer::MaterialPropertyValue::fromBoolean(mHighQualityRendering));
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalHighQualityLighting"), Renderer::MaterialPropertyValue::fromBoolean(mHighQualityLighting));
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalSoftParticles"), Renderer::MaterialPropertyValue::fromBoolean(mSoftParticles));
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalTessellatedTriangleWidth"), Renderer::MaterialPropertyValue::fromFloat(static_cast<float>(mTerrainTessellatedTriangleWidth)));
		// Environment
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalCloudsIntensity"), Renderer::MaterialPropertyValue::fromFloat(mCloudsIntensity));
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalWindDirectionStrength"), Renderer::MaterialPropertyValue::fromFloat4(1.0f, 0.0f, 0.0f, mWindSpeed));
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalUseWetSurfaces"), Renderer::MaterialPropertyValue::fromBoolean(mWetSurfaces[0] > 0.0f));
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalWetSurfaces"), Renderer::MaterialPropertyValue::fromFloat4(mWetSurfaces));
	}

	// Update the scene node rotation
	if (nullptr != mSceneNode && mRotationSpeed > 0.0f)
	{
		glm::vec3 eulerAngles = Renderer::EulerAngles::matrixToEuler(glm::mat3_cast(mSceneNode->getGlobalTransform().rotation));
		eulerAngles.x += renderer.getTimeManager().getPastSecondsSinceLastFrame() * mRotationSpeed;
		mSceneNode->setRotation(Renderer::EulerAngles::eulerToQuaternion(eulerAngles));
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
			const bool hasWindowFocus = (::GetFocus() == reinterpret_cast<HWND>(renderer.getRhi().getContext().getNativeWindowHandle()));
		#else
			bool hasWindowFocus = true;
		#endif
		#ifdef RENDERER_IMGUI
			const bool isAnyWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
		#else
			const bool isAnyWindowHovered = false;
		#endif
		mController->onUpdate(renderer.getTimeManager().getPastSecondsSinceLastFrame(), hasWindowFocus && (mController->isMouseControlInProgress() || !isAnyWindowHovered));
	}

	// Scene hot-reloading memory
	if (nullptr != mCameraSceneItem)
	{
		mHasCameraTransformBackup = true;
		mCameraTransformBackup = mCameraSceneItem->getParentSceneNodeSafe().getGlobalTransform();
	}

	// TODO(co) We need to get informed when the mesh scene item received the mesh resource loading finished signal
	trySetCustomMaterialResource();

	// Update the input system
	mInputManager->Update();

	// Usability: Backup the position and size of the main window so we can restore it in the next session
	#if defined(_WIN32) && defined(RENDERER_IMGUI)
	{
		RECT rect;
		::GetWindowRect(reinterpret_cast<HWND>(renderer.getRhi().getContext().getNativeWindowHandle()), &rect);
		char temp[256];
		sprintf_s(temp, GLM_COUNTOF(temp), "%d %d %d %d", static_cast<int>(rect.left), static_cast<int>(rect.top), static_cast<int>(rect.right - rect.left), static_cast<int>(rect.bottom - rect.top));
		if (INI_NOT_FOUND == mMainWindowPositionSizeIniProperty)
		{
			mMainWindowPositionSizeIniProperty = ini_property_add(mIni, INI_GLOBAL_SECTION, "MainWindowPositionSize", 0, temp, 0);
		}
		else
		{
			ini_property_value_set(mIni, INI_GLOBAL_SECTION, mMainWindowPositionSizeIniProperty, temp, 0);
		}
	}
	#endif
}

void FirstScene::onDraw()
{
	Rhi::IRenderTarget* mainRenderTarget = getMainRenderTarget();
	if (nullptr != mainRenderTarget && nullptr != mCompositorWorkspaceInstance)
	{
		applyCurrentSettings(*mainRenderTarget);
		Renderer::IRenderer& renderer = getRendererSafe();
		Renderer::SceneResource* sceneResource = renderer.getSceneResourceManager().tryGetById(mSceneResourceId);
		if (nullptr != sceneResource && sceneResource->getLoadingState() == Renderer::IResource::LoadingState::LOADED)
		{
			// Flush all queues to have less visible glitches on the first visible frame
			if (mFirstFrame)
			{
				mFirstFrame = false;
				renderer.flushAllQueues();
			}

			// Execute the compositor workspace instance
			createDebugGui(*mainRenderTarget);
			mCompositorWorkspaceInstance->executeVr(*mainRenderTarget, mCameraSceneItem, mSunlightSceneItem);
		}
	}
}


//[-------------------------------------------------------]
//[ Protected virtual Renderer::IResourceListener methods ]
//[-------------------------------------------------------]
void FirstScene::onLoadingStateChange(const Renderer::IResource& resource)
{
	const Renderer::IResource::LoadingState loadingState = resource.getLoadingState();
	if (resource.getAssetId() == ::detail::SCENE_ASSET_ID)
	{
		if (Renderer::IResource::LoadingState::LOADED == loadingState)
		{
			// Sanity checks
			assert(nullptr == mSceneNode);
			assert(nullptr == mCameraSceneItem);
			assert(nullptr == mSunlightSceneItem);
			assert(nullptr == mSkeletonMeshSceneItem);

			// Loop through all scene nodes and grab the first found camera, directional light and mesh
			const Renderer::SceneResource& sceneResource = static_cast<const Renderer::SceneResource&>(resource);
			for (Renderer::SceneNode* sceneNode : sceneResource.getSceneNodes())
			{
				// Loop through all scene items attached to the current scene node
				for (Renderer::ISceneItem* sceneItem : sceneNode->getAttachedSceneItems())
				{
					switch (sceneItem->getSceneItemTypeId())
					{
						case Renderer::MeshSceneItem::TYPE_ID:
							// Grab the first found mesh scene item scene node
							if (nullptr == mSceneNode)
							{
								mSceneNode = sceneNode;
								trySetCustomMaterialResource();
							}
							break;

						case Renderer::CameraSceneItem::TYPE_ID:
							// Grab the first found camera scene item
							if (nullptr == mCameraSceneItem)
							{
								mCameraSceneItem = static_cast<Renderer::CameraSceneItem*>(sceneItem);
								if (mHasCameraTransformBackup)
								{
									// Scene hot-reloading memory
									mCameraSceneItem->getParentSceneNodeSafe().teleportTransform(mCameraTransformBackup);
								}
							}
							break;

						case Renderer::SunlightSceneItem::TYPE_ID:
							// Grab the first found sunlight scene item
							if (nullptr == mSunlightSceneItem)
							{
								mSunlightSceneItem = static_cast<Renderer::SunlightSceneItem*>(sceneItem);
							}
							break;

						case Renderer::SkeletonMeshSceneItem::TYPE_ID:
							// Grab the first found skeleton mesh scene item
							if (nullptr == mSkeletonMeshSceneItem)
							{
								mSkeletonMeshSceneItem = static_cast<Renderer::SkeletonMeshSceneItem*>(sceneItem);
							}
							break;
					}
				}
			}

			if (nullptr != mCameraSceneItem && nullptr != mCameraSceneItem->getParentSceneNode())
			{
				#ifdef RENDERER_OPENVR
					if (mCompositorWorkspaceInstance->getRenderer().getVrManager().isRunning())
					{
						mController = new VrController(*mCameraSceneItem);

						// For VR, set camera to origin
						Renderer::SceneNode* sceneNode = mCameraSceneItem->getParentSceneNode();
						sceneNode->teleportPositionRotation(Renderer::Math::DVEC3_ZERO, Renderer::Math::QUAT_IDENTITY);
					}
					else
				#endif
				{
					mController = new FreeCameraController(*mInputManager, *mCameraSceneItem);

					// Restore camera position and rotation from a previous session if virtual reality is disabled
					#ifdef RENDERER_IMGUI
						if (!mHasCameraTransformBackup && INI_NOT_FOUND != mCameraPositionRotationIniProperty)
						{
							const char* propertyValue = ini_property_value(mIni, INI_GLOBAL_SECTION, mCameraPositionRotationIniProperty);
							if (nullptr != propertyValue)
							{
								glm::dvec3 position = Renderer::Math::DVEC3_ZERO;
								glm::quat rotation = Renderer::Math::QUAT_IDENTITY;
								sscanf(propertyValue, "%lf %lf %lf %f %f %f %f", &position.x, &position.y, &position.z, &rotation.w, &rotation.x, &rotation.y, &rotation.z);
								mCameraSceneItem->getParentSceneNode()->setPositionRotation(position, rotation);
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
	else if (Renderer::IResource::LoadingState::LOADED == loadingState && resource.getAssetId() == ::detail::IMROD_MATERIAL_ASSET_ID)
	{
		// Create our material resource clone
		mCloneMaterialResourceId = getRendererSafe().getMaterialResourceManager().createMaterialResourceByCloning(resource.getId());
		trySetCustomMaterialResource();
	}
}


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
void FirstScene::loadIni()
{
	// Reset ini
	destroyIni();

	// Try to load ini settings from file
	Renderer::IRenderer& renderer = getRendererSafe();
	const Renderer::IFileManager& fileManager = renderer.getFileManager();
	if (fileManager.doesFileExist(::detail::VIRTUAL_SETTINGS_FILENAME))
	{
		Renderer::IFile* file = fileManager.openFile(Renderer::IFileManager::FileMode::READ, ::detail::VIRTUAL_SETTINGS_FILENAME);
		if (nullptr != file)
		{
			mIniFileContent.resize(file->getNumberOfBytes());
			file->read(mIniFileContent.data(), mIniFileContent.size());
			fileManager.closeFile(*file);
			mIni = ini_load(mIniFileContent.data(), &renderer.getContext().getAllocator());
			mMainWindowPositionSizeIniProperty = ini_find_property(mIni, INI_GLOBAL_SECTION, "MainWindowPositionSize", 0);
			mCameraPositionRotationIniProperty = ini_find_property(mIni, INI_GLOBAL_SECTION, "CameraPositionRotation", 0);
			mOpenMetricsWindowIniProperty = ini_find_property(mIni, INI_GLOBAL_SECTION, "OpenMetricsWindow", 0);
		}
	}
	if (nullptr == mIni)
	{
		mIni = ini_create(&renderer.getContext().getAllocator());
	}
}

void FirstScene::saveIni()
{
	if (nullptr != mIni)
	{
		const Renderer::IFileManager& fileManager = getRendererSafe().getFileManager();
		Renderer::IFile* file = fileManager.openFile(Renderer::IFileManager::FileMode::WRITE, ::detail::VIRTUAL_SETTINGS_FILENAME);
		if (nullptr != file)
		{
			Renderer::IRenderer& renderer = getRendererSafe();
			char temp[256];

			// Backup camera position and rotation for a following session, but only if VR isn't running right now
			#ifdef RENDERER_IMGUI
				#ifdef RENDERER_OPENVR
					if (!renderer.getVrManager().isRunning())
				#endif
				{
					sprintf_s(temp, GLM_COUNTOF(temp), "%lf %lf %lf %f %f %f %f", mCameraTransformBackup.position.x, mCameraTransformBackup.position.y, mCameraTransformBackup.position.z, mCameraTransformBackup.rotation.w, mCameraTransformBackup.rotation.x, mCameraTransformBackup.rotation.y, mCameraTransformBackup.rotation.z);
					if (INI_NOT_FOUND == mCameraPositionRotationIniProperty)
					{
						mCameraPositionRotationIniProperty = ini_property_add(mIni, INI_GLOBAL_SECTION, "CameraPositionRotation", 0, temp, 0);
					}
					else
					{
						ini_property_value_set(mIni, INI_GLOBAL_SECTION, mCameraPositionRotationIniProperty, temp, 0);
					}
				}
			#endif

			// Backup open metrics window
			sprintf_s(temp, GLM_COUNTOF(temp), "%d", renderer.getDebugGuiManager().hasOpenMetricsWindow());
			if (INI_NOT_FOUND == mOpenMetricsWindowIniProperty)
			{
				mOpenMetricsWindowIniProperty = ini_property_add(mIni, INI_GLOBAL_SECTION, "OpenMetricsWindow", 0, temp, 0);
			}
			else
			{
				ini_property_value_set(mIni, INI_GLOBAL_SECTION, mOpenMetricsWindowIniProperty, temp, 0);
			}

			// Save ini
			mIniFileContent.resize(static_cast<size_t>(ini_save(mIni, nullptr, 0)));
			ini_save(mIni, mIniFileContent.data(), static_cast<int>(mIniFileContent.size()));
			file->write(mIniFileContent.data(), mIniFileContent.size() - 1);
			fileManager.closeFile(*file);
		}
	}
}

void FirstScene::destroyIni()
{
	if (nullptr != mIni)
	{
		ini_destroy(mIni);
		mIni = nullptr;
		mMainWindowPositionSizeIniProperty = INI_NOT_FOUND;
		mCameraPositionRotationIniProperty = INI_NOT_FOUND;
		mOpenMetricsWindowIniProperty = INI_NOT_FOUND;
	}
}

void FirstScene::applyCurrentSettings(Rhi::IRenderTarget& mainRenderTarget)
{
	if (nullptr != mCompositorWorkspaceInstance && Renderer::isValid(mSceneResourceId))
	{
		// Changes in main swap chain?
		if (mCurrentFullscreen != mFullscreen)
		{
			mCurrentFullscreen = mFullscreen;
			static_cast<Rhi::ISwapChain&>(mainRenderTarget).setFullscreenState(mCurrentFullscreen);
		}
		if (mCurrentUseVerticalSynchronization != mUseVerticalSynchronization)
		{
			mCurrentUseVerticalSynchronization = mUseVerticalSynchronization;
			static_cast<Rhi::ISwapChain&>(mainRenderTarget).setVerticalSynchronizationInterval(mCurrentUseVerticalSynchronization ? 1u : 0u);
		}

		// Recreate the compositor workspace instance, if required
		if (mInstancedCompositor != static_cast<Compositor>(mCurrentCompositor))
		{
			mInstancedCompositor = static_cast<Compositor>(mCurrentCompositor);
			createCompositorWorkspace();
		}

		// Update texture related settings
		Renderer::IRenderer& renderer = getRendererSafe();
		{ // Default texture filtering
			Renderer::MaterialBlueprintResourceManager& materialBlueprintResourceManager = renderer.getMaterialBlueprintResourceManager();
			switch (static_cast<TextureFiltering>(mCurrentTextureFiltering))
			{
				case TextureFiltering::POINT:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Rhi::FilterMode::MIN_MAG_MIP_POINT, 1);
					break;

				case TextureFiltering::BILINEAR:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Rhi::FilterMode::MIN_MAG_LINEAR_MIP_POINT, 1);
					break;

				case TextureFiltering::TRILINEAR:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Rhi::FilterMode::MIN_MAG_MIP_LINEAR, 1);
					break;

				case TextureFiltering::ANISOTROPIC_2:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Rhi::FilterMode::ANISOTROPIC, 2);
					break;

				case TextureFiltering::ANISOTROPIC_4:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Rhi::FilterMode::ANISOTROPIC, 4);
					break;

				case TextureFiltering::ANISOTROPIC_8:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Rhi::FilterMode::ANISOTROPIC, 8);
					break;

				case TextureFiltering::ANISOTROPIC_16:
					materialBlueprintResourceManager.setDefaultTextureFiltering(Rhi::FilterMode::ANISOTROPIC, 16);
					break;
			}
		}
		renderer.getTextureResourceManager().setNumberOfTopMipmapsToRemove(static_cast<uint8_t>(mNumberOfTopTextureMipmapsToRemove));

		// Update mesh related settings
		renderer.getMeshResourceManager().setNumberOfTopMeshLodsToRemove(static_cast<uint8_t>(mNumberOfTopMeshLodsToRemove));

		{ // Update compositor workspace
			const uint8_t maximumNumberOfMultisamples = renderer.getRhi().getCapabilities().maximumNumberOfMultisamples;

			{ // MSAA
				static constexpr uint8_t NUMBER_OF_MULTISAMPLES[4] = { 1, 2, 4, 8 };
				const uint8_t numberOfMultisamples = NUMBER_OF_MULTISAMPLES[mCurrentMsaa];
				mCompositorWorkspaceInstance->setNumberOfMultisamples((numberOfMultisamples > maximumNumberOfMultisamples) ? maximumNumberOfMultisamples : numberOfMultisamples);
			}

			// Resolution Scale
			mCompositorWorkspaceInstance->setResolutionScale(mResolutionScale);

			// Shadow
			if (mShadowQuality != static_cast<ShadowQuality>(mCurrentShadowQuality))
			{
				mShadowQuality = static_cast<ShadowQuality>(mCurrentShadowQuality);
				for (const Renderer::CompositorNodeInstance* compositorNodeInstance : mCompositorWorkspaceInstance->getSequentialCompositorNodeInstances())
				{
					for (Renderer::ICompositorInstancePass* compositorInstancePass : compositorNodeInstance->getCompositorInstancePasses())
					{
						if (compositorInstancePass->getCompositorResourcePass().getTypeId() == Renderer::CompositorResourcePassShadowMap::TYPE_ID)
						{
							Renderer::CompositorInstancePassShadowMap* compositorInstancePassShadowMap = static_cast<Renderer::CompositorInstancePassShadowMap*>(compositorInstancePass);
							switch (mShadowQuality)
							{
								case ShadowQuality::NONE:
									compositorInstancePassShadowMap->setEnabled(false);
									break;

								case ShadowQuality::LOW:
									compositorInstancePassShadowMap->setEnabled(ShadowQuality::NONE != mShadowQuality);
									compositorInstancePassShadowMap->setShadowMapSize(512u);
									compositorInstancePassShadowMap->setNumberOfShadowCascades(2u);
									compositorInstancePassShadowMap->setNumberOfShadowMultisamples(1u);
									break;

								case ShadowQuality::MEDIUM:
									compositorInstancePassShadowMap->setEnabled(ShadowQuality::NONE != mShadowQuality);
									compositorInstancePassShadowMap->setShadowMapSize(1024u);
									compositorInstancePassShadowMap->setNumberOfShadowCascades(2u);
									compositorInstancePassShadowMap->setNumberOfShadowMultisamples(1u);
									break;

								case ShadowQuality::HIGH:
									compositorInstancePassShadowMap->setEnabled(ShadowQuality::NONE != mShadowQuality);
									compositorInstancePassShadowMap->setShadowMapSize(1024u);
									compositorInstancePassShadowMap->setNumberOfShadowCascades(3u);
									compositorInstancePassShadowMap->setNumberOfShadowMultisamples((2u > maximumNumberOfMultisamples) ? maximumNumberOfMultisamples : 2u);
									break;

								case ShadowQuality::ULTRA:
									compositorInstancePassShadowMap->setEnabled(ShadowQuality::NONE != mShadowQuality);
									compositorInstancePassShadowMap->setShadowMapSize(2048u);
									compositorInstancePassShadowMap->setNumberOfShadowCascades(4u);
									compositorInstancePassShadowMap->setNumberOfShadowMultisamples((2 > maximumNumberOfMultisamples) ? maximumNumberOfMultisamples : 2u);
									break;

								case ShadowQuality::EPIC:
									compositorInstancePassShadowMap->setEnabled(ShadowQuality::NONE != mShadowQuality);
									compositorInstancePassShadowMap->setShadowMapSize(4096u);
									compositorInstancePassShadowMap->setNumberOfShadowCascades(4u);
									compositorInstancePassShadowMap->setNumberOfShadowMultisamples((4u > maximumNumberOfMultisamples) ? maximumNumberOfMultisamples : 4u);
									break;
							}

							// We know that there's just a single compositor instance pass shadow map per compositor node instance, so get us out of the inner loop right now
							break;
						}
					}
				}
			}
		}

		{ // Update the material resource instance
			const Renderer::MaterialResourceManager& materialResourceManager = renderer.getMaterialResourceManager();

			// Depth of field compositor material
			Renderer::MaterialResource* materialResource = materialResourceManager.getMaterialResourceByAssetId(ASSET_ID("Example/Blueprint/Compositor/MB_DepthOfField"));
			if (nullptr != materialResource)
			{
				materialResource->setPropertyById(STRING_ID("BlurrinessCutoff"), Renderer::MaterialPropertyValue::fromFloat(mDepthOfFieldBlurrinessCutoff));
			}

			// Final compositor material
			materialResource = materialResourceManager.getMaterialResourceByAssetId(ASSET_ID("Example/Blueprint/Compositor/MB_Final"));
			if (nullptr != materialResource)
			{
				static constexpr uint32_t IDENTITY_TEXTURE_ASSET_ID = ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityColorCorrectionLookupTable3D");
				static constexpr uint32_t SEPIA_TEXTURE_ASSET_ID = ASSET_ID("Example/Blueprint/Compositor/T_SepiaColorCorrectionLookupTable16x1");
				materialResource->setPropertyById(STRING_ID("ColorCorrectionLookupTableMap"), Renderer::MaterialPropertyValue::fromTextureAssetId(mPerformSepiaColorCorrection ? SEPIA_TEXTURE_ASSET_ID : IDENTITY_TEXTURE_ASSET_ID));
				materialResource->setPropertyById(STRING_ID("Fxaa"), Renderer::MaterialPropertyValue::fromBoolean(mPerformFxaa));
				materialResource->setPropertyById(STRING_ID("Sharpen"), Renderer::MaterialPropertyValue::fromBoolean(mPerformSharpen));
				materialResource->setPropertyById(STRING_ID("ChromaticAberration"), Renderer::MaterialPropertyValue::fromBoolean(mPerformChromaticAberration));
				materialResource->setPropertyById(STRING_ID("OldCrtEffect"), Renderer::MaterialPropertyValue::fromBoolean(mPerformOldCrtEffect));
				materialResource->setPropertyById(STRING_ID("FilmGrain"), Renderer::MaterialPropertyValue::fromBoolean(mPerformFilmGrain));
				materialResource->setPropertyById(STRING_ID("Vignette"), Renderer::MaterialPropertyValue::fromBoolean(mPerformVignette));
			}

			// Imrod material clone
			materialResource = materialResourceManager.tryGetById(mCloneMaterialResourceId);
			if (nullptr != materialResource)
			{
				materialResource->setPropertyById(STRING_ID("UseEmissiveMap"), Renderer::MaterialPropertyValue::fromBoolean(mUseEmissiveMap));
				materialResource->setPropertyById(STRING_ID("AlbedoColor"), Renderer::MaterialPropertyValue::fromFloat3(mAlbedoColor));
			}
		}
	}
}

void FirstScene::createCompositorWorkspace()
{
	// Create/recreate the compositor workspace instance
	static constexpr uint32_t COMPOSITOR_WORKSPACE_ASSET_ID[4] =
	{
		ASSET_ID("Example/CompositorWorkspace/CW_Debug"),
		ASSET_ID("Example/CompositorWorkspace/CW_Forward"),
		ASSET_ID("Example/CompositorWorkspace/CW_Deferred"),
		ASSET_ID("Example/CompositorWorkspace/CW_Vr")
	};
	delete mCompositorWorkspaceInstance;
	mCompositorWorkspaceInstance = new Renderer::CompositorWorkspaceInstance(getRendererSafe(), COMPOSITOR_WORKSPACE_ASSET_ID[static_cast<int>(mInstancedCompositor)]);
}

void FirstScene::createDebugGui([[maybe_unused]] Rhi::IRenderTarget& mainRenderTarget)
{
	#ifdef RENDERER_IMGUI
		if (nullptr != mCompositorWorkspaceInstance && Renderer::isValid(mSceneResourceId))
		{
			// Get the render target the debug GUI is rendered into, use the provided main render target as fallback
			const Renderer::ICompositorInstancePass* compositorInstancePass = mCompositorWorkspaceInstance->getFirstCompositorInstancePassByCompositorPassTypeId(Renderer::CompositorResourcePassDebugGui::TYPE_ID);
			if (nullptr != compositorInstancePass)
			{
				// Setup GUI
				Renderer::IRenderer& renderer = getRendererSafe();
				Renderer::DebugGuiManager& debugGuiManager = renderer.getDebugGuiManager();
				debugGuiManager.newFrame(((nullptr != compositorInstancePass->getRenderTarget()) ? *compositorInstancePass->getRenderTarget() : mainRenderTarget), mCompositorWorkspaceInstance);
				mImGuiLog->draw(renderer.getContext().getFileManager());
				if (ImGui::Begin("Options"))
				{
					// Status
					static const ImVec4 GREY_COLOR(0.5f, 0.5f, 0.5f, 1.0f);
					static const ImVec4 RED_COLOR(1.0f, 0.0f, 0.0f, 1.0f);
					ImGui::PushStyleColor(ImGuiCol_Text, GREY_COLOR);
						ImGui::Text("RHI: %s", mainRenderTarget.getRhi().getName());
						ImGui::Text("GPU: %s", mainRenderTarget.getRhi().getCapabilities().deviceName);
						#ifdef RENDERER_TOOLKIT
						{ // Renderer toolkit
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
							const bool idle = (0 == renderer.getResourceStreamer().getNumberOfInFlightLoadRequests());
							ImGui::PushStyleColor(ImGuiCol_Text, idle ? GREY_COLOR : RED_COLOR);
								ImGui::Text("Resource Streamer: %s", idle ? "Idle" : "Busy");
							ImGui::PopStyleColor();
						}
						{ // Pipeline state compiler
							const bool idle = (0 == renderer.getGraphicsPipelineStateCompiler().getNumberOfInFlightCompilerRequests() && 0 == renderer.getComputePipelineStateCompiler().getNumberOfInFlightCompilerRequests());
							ImGui::PushStyleColor(ImGuiCol_Text, idle ? GREY_COLOR : RED_COLOR);
								ImGui::Text("Pipeline State Compiler: %s", idle ? "Idle" : "Busy");
							ImGui::PopStyleColor();
						}
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
					#ifdef RENDERER_GRAPHICS_DEBUGGER
					{
						Renderer::IGraphicsDebugger& graphicsDebugger = renderer.getContext().getGraphicsDebugger();
						if (graphicsDebugger.isInitialized())
						{
							ImGui::SameLine();
							if (ImGui::Button("Capture"))
							{
								graphicsDebugger.captureNextFrame();
							}
							if (ImGui::IsItemHovered())
							{
								ImGui::SetTooltip("Capture next frame for RenderDoc graphics debugging");
							}
						}
					}
					#endif
					ImGui::Separator();

					// Video
					if (ImGui::BeginMenu("Video"))
					{
						// TODO(co) Add fullscreen combo box (window, borderless window, native fullscreen)
						mFullscreen = static_cast<Rhi::ISwapChain&>(mainRenderTarget).getFullscreenState();	// It's possible to toggle fullscreen by using ALT-return, take this into account
						ImGui::Checkbox("Fullscreen", &mFullscreen);
						// TODO(co) Add resolution and refresh rate combo box
						ImGui::SliderFloat("Resolution Scale", &mResolutionScale, 0.05f, 4.0f, "%.3f");
						ImGui::Checkbox("Vertical Synchronization", &mUseVerticalSynchronization);
						if (renderer.getRhi().getCapabilities().maximumNumberOfMultisamples > 1)
						{
							static constexpr const char* items[] = { "None", "2x", "4x", "8x" };
							ImGui::Combo("MSAA", &mCurrentMsaa, items, static_cast<int>(GLM_COUNTOF(items)));
						}
						ImGui::EndMenu();
					}

					// Graphics
					if (ImGui::BeginMenu("Graphics"))
					{
						{ // Compositor
							static constexpr const char* items[] = { "Debug", "Forward", "Deferred", "VR" };
							ImGui::Combo("Compositor", &mCurrentCompositor, items, static_cast<int>(GLM_COUNTOF(items)));
						}
						{ // Shadow quality
							static constexpr const char* items[] = { "None", "Low", "Medium", "High", "Ultra", "Epic" };
							ImGui::Combo("Shadow Quality", &mCurrentShadowQuality, items, static_cast<int>(GLM_COUNTOF(items)));
						}
						{ // High quality rendering
							ImGui::Checkbox("High Quality Rendering", &mHighQualityRendering);
							if (ImGui::IsItemHovered())
							{
								ImGui::SetTooltip("High quality rendering enables e.g. terrain triplanar mapping");
							}
						}
						ImGui::Checkbox("High Quality Lighting", &mHighQualityLighting);
						ImGui::Checkbox("Soft-Particles", &mSoftParticles);
						{ // Texture filtering
							static constexpr const char* items[] = { "Point", "Bilinear", "Trilinear", "2x Anisotropic", "4x Anisotropic", "8x Anisotropic", "16x Anisotropic" };
							ImGui::Combo("Texture Filtering", &mCurrentTextureFiltering, items, static_cast<int>(GLM_COUNTOF(items)));
						}
						ImGui::SliderInt("Texture Mipmaps to Remove", &mNumberOfTopTextureMipmapsToRemove, 0, 8);
						ImGui::SliderInt("Mesh LODs to Remove", &mNumberOfTopMeshLodsToRemove, 0, 4);
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
							Renderer::DebugGuiHelper::drawSkeleton(*mCameraSceneItem, *mSkeletonMeshSceneItem);
						}

						// Scene node transform using gizmo
						if (nullptr != mSceneNode)
						{
							// Draw gizmo
							ImGui::Separator();
							Renderer::Transform transform = mSceneNode->getGlobalTransform();
							Renderer::DebugGuiHelper::drawGizmo(*mCameraSceneItem, mGizmoSettings, transform);
							mSceneNode->setTransform(transform);

							// Draw grid
							// TODO(co) Make this optional via GUI
							// Renderer::DebugGuiHelper::drawGrid(*mCameraSceneItem, transform.position.y);
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
	if (!mCustomMaterialResourceSet && nullptr != mSceneNode && Renderer::isValid(mCloneMaterialResourceId))
	{
		for (Renderer::ISceneItem* sceneItem : mSceneNode->getAttachedSceneItems())
		{
			if (sceneItem->getSceneItemTypeId() == Renderer::MeshSceneItem::TYPE_ID)
			{
				// Tell the mesh scene item about our custom material resource
				Renderer::MeshSceneItem* meshSceneItem = static_cast<Renderer::MeshSceneItem*>(sceneItem);
				if (Renderer::IResource::LoadingState::LOADED == getRendererSafe().getMeshResourceManager().getResourceByResourceId(meshSceneItem->getMeshResourceId()).getLoadingState())
				{
					meshSceneItem->setMaterialResourceIdOfAllSubMeshesAndLods(mCloneMaterialResourceId);
					mCustomMaterialResourceSet = true;
				}
			}
		}
	}
}
