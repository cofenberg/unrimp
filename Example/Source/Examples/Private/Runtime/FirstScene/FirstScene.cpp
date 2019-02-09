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
#include "Examples/Private/Runtime/FirstScene/FirstScene.h"
#include "Examples/Private/Runtime/FirstScene/FreeCameraController.h"
#ifdef RENDERER_RUNTIME_OPENVR
	#include "Examples/Private/Runtime/FirstScene/VrController.h"
#endif
#ifdef _WIN32
	#include <RendererRuntime/Public/Core/Platform/WindowsHeader.h>
#endif

#ifdef RENDERER_TOOLKIT
	#include <RendererToolkit/Public/RendererToolkit.h>
#endif

#include <RendererRuntime/Public/IRendererRuntime.h>
#include <RendererRuntime/Public/Core/Math/EulerAngles.h>
#include <RendererRuntime/Public/Core/Time/TimeManager.h>
#ifdef RENDERER_RUNTIME_GRAPHICS_DEBUGGER
	#include <RendererRuntime/Public/Core/RenderDocGraphicsDebugger.h>
#endif
#ifdef RENDERER_RUNTIME_IMGUI
	#include <RendererRuntime/Public/DebugGui/ImGuiLog.h>
	#include <RendererRuntime/Public/DebugGui/DebugGuiManager.h>
#endif
#ifdef RENDERER_RUNTIME_OPENVR
	#include <RendererRuntime/Public/Vr/IVrManager.h>
#endif
#include <RendererRuntime/Public/Resource/Scene/SceneNode.h>
#include <RendererRuntime/Public/Resource/Scene/SceneResource.h>
#include <RendererRuntime/Public/Resource/Scene/SceneResourceManager.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Camera/CameraSceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Light/SunlightSceneItem.h>
#include <RendererRuntime/Public/Resource/Scene/Item/Mesh/SkeletonMeshSceneItem.h>
#include <RendererRuntime/Public/Resource/Mesh/MeshResourceManager.h>
#include <RendererRuntime/Public/Resource/CompositorNode/Pass/DebugGui/CompositorResourcePassDebugGui.h>
#include <RendererRuntime/Public/Resource/CompositorNode/Pass/ShadowMap/CompositorInstancePassShadowMap.h>
#include <RendererRuntime/Public/Resource/CompositorWorkspace/CompositorWorkspaceInstance.h>
#include <RendererRuntime/Public/Resource/MaterialBlueprint/Cache/GraphicsPipelineStateCompiler.h>
#include <RendererRuntime/Public/Resource/MaterialBlueprint/Cache/ComputePipelineStateCompiler.h>
#include <RendererRuntime/Public/Resource/MaterialBlueprint/MaterialBlueprintResourceManager.h>
#include <RendererRuntime/Public/Resource/Material/MaterialResourceManager.h>
#include <RendererRuntime/Public/Resource/Material/MaterialResource.h>
#include <RendererRuntime/Public/Resource/Texture/TextureResourceManager.h>
#include <RendererRuntime/Public/Resource/ResourceStreamer.h>
#include <RendererRuntime/Public/Context.h>

#include <Renderer/Public/DefaultAllocator.h>

#include <DeviceInput/DeviceInput.h>

#define INI_IMPLEMENTATION
#define INI_MALLOC(ctx, size) (static_cast<Renderer::IAllocator*>(ctx)->reallocate(nullptr, 0, size, 1))
#define INI_FREE(ctx, ptr) (static_cast<Renderer::IAllocator*>(ctx)->reallocate(ptr, 0, 0, 1))
#include <ini/ini.h>

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
	loadIni();

	// Get and check the renderer runtime instance
	RendererRuntime::IRendererRuntime& rendererRuntime = getRendererRuntimeSafe();

	// Usability: Restore the position and size of the main window from a previous session
	#if defined(_WIN32) && !defined(SDL2_FOUND) && defined(RENDERER_RUNTIME_IMGUI)
		if (INI_NOT_FOUND != mMainWindowPositionSizeIniProperty)
		{
			const char* propertyValue = ini_property_value(mIni, INI_GLOBAL_SECTION, mMainWindowPositionSizeIniProperty);
			if (nullptr != propertyValue)
			{
				int value[4] = { 0, 0, 1024, 768 };
				sscanf(propertyValue, "%d %d %d %d", &value[0], &value[1], &value[2], &value[3]);
				::SetWindowPos(reinterpret_cast<HWND>(rendererRuntime.getRenderer().getContext().getNativeWindowHandle()), HWND_TOP, static_cast<int>(value[0]), static_cast<int>(value[1]), static_cast<int>(value[2]), static_cast<int>(value[3]), 0);
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
				rendererRuntime.getDebugGuiManager().openMetricsWindow();
			}
		}
	}

	// TODO(co) Remove this after the Vulkan renderer backend is fully up-and-running. Or better, add asset properties so one can e.g. add asset information regarding e.g. supported renderer backends.
	const Renderer::NameId nameId = rendererRuntime.getRenderer().getNameId();
	if (Renderer::NameId::VULKAN == nameId || Renderer::NameId::DIRECT3D10 == nameId || Renderer::NameId::DIRECT3D9 == nameId)
	{
		mInstancedCompositor = Compositor::DEBUG;
		mCurrentCompositor = static_cast<int>(mInstancedCompositor);
		if (Renderer::NameId::VULKAN == nameId)
		{
			rendererRuntime.getMaterialBlueprintResourceManager().setCreateInitialPipelineStateCaches(false);
		}
	}

	// Create the scene resource
	rendererRuntime.getSceneResourceManager().loadSceneResourceByAssetId(::detail::SCENE_ASSET_ID, mSceneResourceId, this);

	// Load the material resource we're going to clone
	rendererRuntime.getMaterialResourceManager().loadMaterialResourceByAssetId(::detail::IMROD_MATERIAL_ASSET_ID, mMaterialResourceId, this);

	// Try to startup the VR-manager if a HMD is present
	#ifdef RENDERER_RUNTIME_OPENVR
	{
		RendererRuntime::IVrManager& vrManager = rendererRuntime.getVrManager();
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
	// TODO(co) The Vulkan renderer backend is under construction, so debug compositor for now
	if (rendererRuntime.getRenderer().getNameId() == Renderer::NameId::VULKAN || rendererRuntime.getRenderer().getNameId() == Renderer::NameId::OPENGLES3)
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
	getRendererRuntimeSafe().getSceneResourceManager().destroySceneResource(mSceneResourceId);
	RendererRuntime::setInvalid(mSceneResourceId);

	// Destroy controller instance
	if (nullptr != mController)
	{
		delete mController;
		mController = nullptr;
	}
}

void FirstScene::onUpdate()
{
	const RendererRuntime::IRendererRuntime& rendererRuntime = getRendererRuntimeSafe();

	{ // Tell the material blueprint resource manager about our global material properties
		RendererRuntime::MaterialProperties& globalMaterialProperties = rendererRuntime.getMaterialBlueprintResourceManager().getGlobalMaterialProperties();
		// Graphics
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalReceiveShadows"), RendererRuntime::MaterialPropertyValue::fromBoolean(ShadowQuality::NONE != mShadowQuality));
		globalMaterialProperties.setPropertyById(STRING_ID("GlobalHighQualityRendering"), RendererRuntime::MaterialPropertyValue::fromBoolean(mHighQualityRendering));
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
		eulerAngles.x += rendererRuntime.getTimeManager().getPastSecondsSinceLastFrame() * mRotationSpeed;
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
			const bool hasWindowFocus = (::GetFocus() == reinterpret_cast<HWND>(rendererRuntime.getRenderer().getContext().getNativeWindowHandle()));
		#else
			bool hasWindowFocus = true;
		#endif
		#ifdef RENDERER_RUNTIME_IMGUI
			const bool isAnyWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
		#else
			const bool isAnyWindowHovered = false;
		#endif
		mController->onUpdate(rendererRuntime.getTimeManager().getPastSecondsSinceLastFrame(), hasWindowFocus && (mController->isMouseControlInProgress() || !isAnyWindowHovered));
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
	#if defined(_WIN32) && defined(RENDERER_RUNTIME_IMGUI)
	{
		RECT rect;
		::GetWindowRect(reinterpret_cast<HWND>(rendererRuntime.getRenderer().getContext().getNativeWindowHandle()), &rect);
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
	Renderer::IRenderTarget* mainRenderTarget = getMainRenderTarget();
	if (nullptr != mainRenderTarget && nullptr != mCompositorWorkspaceInstance)
	{
		applyCurrentSettings(*mainRenderTarget);
		RendererRuntime::IRendererRuntime& rendererRuntime = getRendererRuntimeSafe();
		RendererRuntime::SceneResource* sceneResource = rendererRuntime.getSceneResourceManager().tryGetById(mSceneResourceId);
		if (nullptr != sceneResource && sceneResource->getLoadingState() == RendererRuntime::IResource::LoadingState::LOADED)
		{
			// Flush all queues to have less visible glitches on the first visible frame
			if (mFirstFrame)
			{
				mFirstFrame = false;
				rendererRuntime.flushAllQueues();
			}

			// Execute the compositor workspace instance
			createDebugGui(*mainRenderTarget);
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
									mCameraSceneItem->getParentSceneNodeSafe().teleportTransform(mCameraTransformBackup);
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
						sceneNode->teleportPositionRotation(RendererRuntime::Math::DVEC3_ZERO, RendererRuntime::Math::QUAT_IDENTITY);
					}
					else
				#endif
				{
					mController = new FreeCameraController(*mInputManager, *mCameraSceneItem);

					// Restore camera position and rotation from a previous session if virtual reality is disabled
					#ifdef RENDERER_RUNTIME_IMGUI
						if (!mHasCameraTransformBackup && INI_NOT_FOUND != mCameraPositionRotationIniProperty)
						{
							const char* propertyValue = ini_property_value(mIni, INI_GLOBAL_SECTION, mCameraPositionRotationIniProperty);
							if (nullptr != propertyValue)
							{
								glm::dvec3 position = RendererRuntime::Math::DVEC3_ZERO;
								glm::quat rotation = RendererRuntime::Math::QUAT_IDENTITY;
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
	else if (RendererRuntime::IResource::LoadingState::LOADED == loadingState && resource.getAssetId() == ::detail::IMROD_MATERIAL_ASSET_ID)
	{
		// Create our material resource clone
		mCloneMaterialResourceId = getRendererRuntimeSafe().getMaterialResourceManager().createMaterialResourceByCloning(resource.getId());
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
	RendererRuntime::IRendererRuntime& rendererRuntime = getRendererRuntimeSafe();
	const RendererRuntime::IFileManager& fileManager = rendererRuntime.getFileManager();
	if (fileManager.doesFileExist(::detail::VIRTUAL_SETTINGS_FILENAME))
	{
		RendererRuntime::IFile* file = fileManager.openFile(RendererRuntime::IFileManager::FileMode::READ, ::detail::VIRTUAL_SETTINGS_FILENAME);
		if (nullptr != file)
		{
			mIniFileContent.resize(file->getNumberOfBytes());
			file->read(mIniFileContent.data(), mIniFileContent.size());
			fileManager.closeFile(*file);
			mIni = ini_load(mIniFileContent.data(), &rendererRuntime.getContext().getAllocator());
			mMainWindowPositionSizeIniProperty = ini_find_property(mIni, INI_GLOBAL_SECTION, "MainWindowPositionSize", 0);
			mCameraPositionRotationIniProperty = ini_find_property(mIni, INI_GLOBAL_SECTION, "CameraPositionRotation", 0);
			mOpenMetricsWindowIniProperty = ini_find_property(mIni, INI_GLOBAL_SECTION, "OpenMetricsWindow", 0);
		}
	}
	if (nullptr == mIni)
	{
		mIni = ini_create(&rendererRuntime.getContext().getAllocator());
	}
}

void FirstScene::saveIni()
{
	if (nullptr != mIni)
	{
		const RendererRuntime::IFileManager& fileManager = getRendererRuntimeSafe().getFileManager();
		RendererRuntime::IFile* file = fileManager.openFile(RendererRuntime::IFileManager::FileMode::WRITE, ::detail::VIRTUAL_SETTINGS_FILENAME);
		if (nullptr != file)
		{
			RendererRuntime::IRendererRuntime& rendererRuntime = getRendererRuntimeSafe();
			char temp[256];

			// Backup camera position and rotation for a following session, but only if VR isn't running right now
			#ifdef RENDERER_RUNTIME_IMGUI
				#ifdef RENDERER_RUNTIME_OPENVR
					if (!rendererRuntime.getVrManager().isRunning())
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
			sprintf_s(temp, GLM_COUNTOF(temp), "%d", rendererRuntime.getDebugGuiManager().hasOpenMetricsWindow());
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

void FirstScene::applyCurrentSettings(Renderer::IRenderTarget& mainRenderTarget)
{
	if (nullptr != mCompositorWorkspaceInstance && RendererRuntime::isValid(mSceneResourceId))
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
		if (mInstancedCompositor != static_cast<Compositor>(mCurrentCompositor))
		{
			mInstancedCompositor = static_cast<Compositor>(mCurrentCompositor);
			createCompositorWorkspace();
		}

		// Update texture related settings
		RendererRuntime::IRendererRuntime& rendererRuntime = getRendererRuntimeSafe();
		{ // Default texture filtering
			RendererRuntime::MaterialBlueprintResourceManager& materialBlueprintResourceManager = rendererRuntime.getMaterialBlueprintResourceManager();
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
		rendererRuntime.getTextureResourceManager().setNumberOfTopMipmapsToRemove(static_cast<uint8_t>(mNumberOfTopTextureMipmapsToRemove));

		{ // Update compositor workspace
			const uint8_t maximumNumberOfMultisamples = rendererRuntime.getRenderer().getCapabilities().maximumNumberOfMultisamples;

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
				for (const RendererRuntime::CompositorNodeInstance* compositorNodeInstance : mCompositorWorkspaceInstance->getSequentialCompositorNodeInstances())
				{
					for (RendererRuntime::ICompositorInstancePass* compositorInstancePass : compositorNodeInstance->getCompositorInstancePasses())
					{
						if (compositorInstancePass->getCompositorResourcePass().getTypeId() == RendererRuntime::CompositorResourcePassShadowMap::TYPE_ID)
						{
							RendererRuntime::CompositorInstancePassShadowMap* compositorInstancePassShadowMap = static_cast<RendererRuntime::CompositorInstancePassShadowMap*>(compositorInstancePass);
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
			const RendererRuntime::MaterialResourceManager& materialResourceManager = rendererRuntime.getMaterialResourceManager();

			// Depth of field compositor material
			RendererRuntime::MaterialResource* materialResource = materialResourceManager.getMaterialResourceByAssetId(ASSET_ID("Example/Blueprint/Compositor/MB_DepthOfField"));
			if (nullptr != materialResource)
			{
				materialResource->setPropertyById(STRING_ID("BlurrinessCutoff"), RendererRuntime::MaterialPropertyValue::fromFloat(mDepthOfFieldBlurrinessCutoff));
			}

			// Final compositor material
			materialResource = materialResourceManager.getMaterialResourceByAssetId(ASSET_ID("Example/Blueprint/Compositor/MB_Final"));
			if (nullptr != materialResource)
			{
				static constexpr uint32_t IDENTITY_TEXTURE_ASSET_ID = ASSET_ID("Unrimp/Texture/DynamicByCode/IdentityColorCorrectionLookupTable3D");
				static constexpr uint32_t SEPIA_TEXTURE_ASSET_ID = ASSET_ID("Example/Blueprint/Compositor/T_SepiaColorCorrectionLookupTable16x1");
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
	// Create/recreate the compositor workspace instance
	static constexpr uint32_t COMPOSITOR_WORKSPACE_ASSET_ID[4] =
	{
		ASSET_ID("Example/CompositorWorkspace/CW_Debug"),
		ASSET_ID("Example/CompositorWorkspace/CW_Forward"),
		ASSET_ID("Example/CompositorWorkspace/CW_Deferred"),
		ASSET_ID("Example/CompositorWorkspace/CW_Vr")
	};
	delete mCompositorWorkspaceInstance;
	mCompositorWorkspaceInstance = new RendererRuntime::CompositorWorkspaceInstance(getRendererRuntimeSafe(), COMPOSITOR_WORKSPACE_ASSET_ID[static_cast<int>(mInstancedCompositor)]);
}

void FirstScene::createDebugGui([[maybe_unused]] Renderer::IRenderTarget& mainRenderTarget)
{
	#ifdef RENDERER_RUNTIME_IMGUI
		if (nullptr != mCompositorWorkspaceInstance && RendererRuntime::isValid(mSceneResourceId))
		{
			// Get the render target the debug GUI is rendered into, use the provided main render target as fallback
			const RendererRuntime::ICompositorInstancePass* compositorInstancePass = mCompositorWorkspaceInstance->getFirstCompositorInstancePassByCompositorPassTypeId(RendererRuntime::CompositorResourcePassDebugGui::TYPE_ID);
			if (nullptr != compositorInstancePass)
			{
				// Setup GUI
				RendererRuntime::IRendererRuntime& rendererRuntime = getRendererRuntimeSafe();
				RendererRuntime::DebugGuiManager& debugGuiManager = rendererRuntime.getDebugGuiManager();
				debugGuiManager.newFrame(((nullptr != compositorInstancePass->getRenderTarget()) ? *compositorInstancePass->getRenderTarget() : mainRenderTarget), mCompositorWorkspaceInstance);
				mImGuiLog->draw(rendererRuntime.getContext().getFileManager());
				if (ImGui::Begin("Options"))
				{
					// Status
					static const ImVec4 GREY_COLOR(0.5f, 0.5f, 0.5f, 1.0f);
					static const ImVec4 RED_COLOR(1.0f, 0.0f, 0.0f, 1.0f);
					ImGui::PushStyleColor(ImGuiCol_Text, GREY_COLOR);
						ImGui::Text("Renderer: %s", mainRenderTarget.getRenderer().getName());
						ImGui::Text("GPU: %s", mainRenderTarget.getRenderer().getCapabilities().deviceName);
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
							const bool idle = (0 == rendererRuntime.getResourceStreamer().getNumberOfInFlightLoadRequests());
							ImGui::PushStyleColor(ImGuiCol_Text, idle ? GREY_COLOR : RED_COLOR);
								ImGui::Text("Resource Streamer: %s", idle ? "Idle" : "Busy");
							ImGui::PopStyleColor();
						}
						{ // Pipeline state compiler
							const bool idle = (0 == rendererRuntime.getGraphicsPipelineStateCompiler().getNumberOfInFlightCompilerRequests() && 0 == rendererRuntime.getComputePipelineStateCompiler().getNumberOfInFlightCompilerRequests());
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
					#ifdef RENDERER_RUNTIME_GRAPHICS_DEBUGGER
					{
						RendererRuntime::IGraphicsDebugger& graphicsDebugger = rendererRuntime.getContext().getGraphicsDebugger();
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
						mFullscreen = static_cast<Renderer::ISwapChain&>(mainRenderTarget).getFullscreenState();	// It's possible to toggle fullscreen by using ALT-return, take this into account
						ImGui::Checkbox("Fullscreen", &mFullscreen);
						// TODO(co) Add resolution and refresh rate combo box
						ImGui::SliderFloat("Resolution Scale", &mResolutionScale, 0.05f, 4.0f, "%.3f");
						ImGui::Checkbox("Vertical Synchronization", &mUseVerticalSynchronization);
						if (rendererRuntime.getRenderer().getCapabilities().maximumNumberOfMultisamples > 1)
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
		for (RendererRuntime::ISceneItem* sceneItem : mSceneNode->getAttachedSceneItems())
		{
			if (sceneItem->getSceneItemTypeId() == RendererRuntime::MeshSceneItem::TYPE_ID)
			{
				// Tell the mesh scene item about our custom material resource
				RendererRuntime::MeshSceneItem* meshSceneItem = static_cast<RendererRuntime::MeshSceneItem*>(sceneItem);
				if (RendererRuntime::IResource::LoadingState::LOADED == getRendererRuntimeSafe().getMeshResourceManager().getResourceByResourceId(meshSceneItem->getMeshResourceId()).getLoadingState())
				{
					meshSceneItem->setMaterialResourceIdOfAllSubMeshes(mCloneMaterialResourceId);
					mCustomMaterialResourceSet = true;
				}
			}
		}
	}
}
