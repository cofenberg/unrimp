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
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "Examples/Private/Framework/ExampleBase.h"

#include <RendererRuntime/Public/Core/Math/Transform.h>
#include <RendererRuntime/Public/DebugGui/DebugGuiHelper.h>
#include <RendererRuntime/Public/Resource/IResourceListener.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
class IController;
namespace DeviceInput
{
	class InputManager;
}
namespace RendererRuntime
{
	class ImGuiLog;
	class SceneNode;
	class CameraSceneItem;
	class SunlightSceneItem;
	class SkeletonMeshSceneItem;
	class CompositorWorkspaceInstance;
}


//[-------------------------------------------------------]
//[ Global definitions                                    ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	typedef uint32_t SceneResourceId;		///< POD scene resource identifier
	typedef uint32_t MaterialResourceId;	///< POD material resource identifier
}


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    First scene example
*
*  @remarks
*    Demonstrates:
*    - Compositor
*    - Scene
*    - Virtual reality (VR)
*/
class FirstScene final : public ExampleBase, public RendererRuntime::IResourceListener
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*/
	FirstScene();

	/**
	*  @brief
	*    Destructor
	*/
	virtual ~FirstScene() override;


//[-------------------------------------------------------]
//[ Public virtual ExampleBase methods                    ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization() override;
	virtual void onDeinitialization() override;
	virtual void onUpdate() override;
	virtual void onDraw() override;

	[[nodiscard]] inline virtual bool doesCompleteOwnDrawing() const override
	{
		// This example wants complete control of the drawing
		return true;
	}


//[-------------------------------------------------------]
//[ Protected virtual RendererRuntime::IResourceListener methods ]
//[-------------------------------------------------------]
protected:
	virtual void onLoadingStateChange(const RendererRuntime::IResource& resource) override;


//[-------------------------------------------------------]
//[ Private methods                                       ]
//[-------------------------------------------------------]
private:
	explicit FirstScene(const FirstScene&) = delete;
	FirstScene& operator=(const FirstScene&) = delete;
	void applyCurrentSettings(Renderer::IRenderTarget& mainRenderTarget);
	void createCompositorWorkspace();
	void createDebugGui(Renderer::IRenderTarget& mainRenderTarget);
	void trySetCustomMaterialResource();


//[-------------------------------------------------------]
//[ Private definitions                                   ]
//[-------------------------------------------------------]
private:
	enum Compositor
	{
		DEBUG,
		FORWARD,
		DEFERRED,
		VR
	};
	enum Msaa
	{
		NONE,
		TWO,
		FOUR,
		EIGHT
	};
	enum TextureFiltering
	{
		POINT,
		BILINEAR,
		TRILINEAR,
		ANISOTROPIC_2,
		ANISOTROPIC_4,
		ANISOTROPIC_8,
		ANISOTROPIC_16
	};


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	DeviceInput::InputManager*					  mInputManager;
	RendererRuntime::ImGuiLog*					  mImGuiLog;
	RendererRuntime::CompositorWorkspaceInstance* mCompositorWorkspaceInstance;
	bool										  mFirstFrame;
	RendererRuntime::SceneResourceId			  mSceneResourceId;
	RendererRuntime::MaterialResourceId			  mMaterialResourceId;
	RendererRuntime::MaterialResourceId			  mCloneMaterialResourceId;
	bool										  mCustomMaterialResourceSet;
	IController*								  mController;
	// Crazy raw-pointers to point-of-interest scene stuff
	RendererRuntime::CameraSceneItem*		mCameraSceneItem;
	RendererRuntime::SunlightSceneItem*		mSunlightSceneItem;
	RendererRuntime::SkeletonMeshSceneItem*	mSkeletonMeshSceneItem;
	RendererRuntime::SceneNode*				mSceneNode;
	// States for runtime-editing
	RendererRuntime::DebugGuiHelper::GizmoSettings mGizmoSettings;
	// Video
	bool  mFullscreen;
	bool  mCurrentFullscreen;
	float mResolutionScale;
	bool  mUseVerticalSynchronization;
	bool  mCurrentUseVerticalSynchronization;
	int	  mCurrentMsaa;
	// Graphics
	Compositor mInstancedCompositor;
	int		   mCurrentCompositor;
	bool	   mHighQualityLighting;
	bool	   mSoftParticles;
	int		   mCurrentTextureFiltering;
	int		   mNumberOfTopTextureMipmapsToRemove;
	int		   mTerrainTessellatedTriangleWidth;
	// Environment
	float mCloudsIntensity;
	float mWindSpeed;
	float mWetSurfaces[4];	// x=wet level, y=hole/cracks flood level, z=puddle flood level, w=rain intensity
	// Post processing
	bool  mPerformFxaa;
	bool  mPerformSharpen;
	bool  mPerformChromaticAberration;
	bool  mPerformOldCrtEffect;
	bool  mPerformFilmGrain;
	bool  mPerformSepiaColorCorrection;
	bool  mPerformVignette;
	float mDepthOfFieldBlurrinessCutoff;
	// Selected material properties
	bool  mUseEmissiveMap;
	float mAlbedoColor[3];
	// Selected scene item
	float mRotationSpeed;
	bool  mShowSkeleton;
	// Scene hot-reloading memory
	bool					   mHasCameraTransformBackup;
	RendererRuntime::Transform mCameraTransformBackup;


};
