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
#include "Advanced/InstancedCubes/InstancedCubes.h"
#include "Advanced/InstancedCubes/CubeRendererDrawInstanced/CubeRendererDrawInstanced.h"
#include "Advanced/InstancedCubes/CubeRendererInstancedArrays/CubeRendererInstancedArrays.h"
#include "Framework/IApplication.h"
#include "Framework/Color4.h"

#ifdef RENDERER_RUNTIME_IMGUI
	#include <RendererRuntime/IRendererRuntime.h>
	#include <RendererRuntime/DebugGui/DebugGuiManager.h>
	#include <RendererRuntime/DebugGui/DebugGuiHelper.h>
#endif

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <glm/gtc/type_ptr.hpp>
	#include <glm/gtc/matrix_transform.hpp>
PRAGMA_WARNING_POP

#include <DeviceInput/DeviceInput.h>

#include <math.h>
#include <stdio.h>
#include <limits.h>


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
InstancedCubes::InstancedCubes() :
	mInputManager(new DeviceInput::InputManager()),
	mCubeRenderer(nullptr),
	mNumberOfCubeInstances(1000),
	mGlobalTimer(0.0f),
	mGlobalScale(1.0f),
	mDisplayStatistics(true),
	mFPSUpdateTimer(0.0f),
	mFramesSinceCheck(0),
	mFramesPerSecond(0.0f)
{
	// Nothing here
}

InstancedCubes::~InstancedCubes()
{
	delete mInputManager;

	// The resources are released within "onDeinitialization()"
}


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
void InstancedCubes::onInitialization()
{
	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Create the cube renderer instance
		// -> Evaluate the feature set of the used renderer
		// TODO(co) This example doesn't support texture buffer emulation, which for OpenGL ES 3 is currently used
		const Renderer::Capabilities& capabilities = renderer->getCapabilities();
		if (capabilities.drawInstanced && capabilities.maximumNumberOf2DTextureArraySlices > 0 && capabilities.maximumTextureBufferSize > 0 && renderer->getNameId() != Renderer::NameId::OPENGLES3)
		{
			// Render cubes by using draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
			mCubeRenderer = new CubeRendererDrawInstanced(*renderer, getMainRenderTarget()->getRenderPass(), NUMBER_OF_TEXTURES, SCENE_RADIUS);
		}
		else if (capabilities.instancedArrays)
		{
			// Render cubes by using instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
			mCubeRenderer = new CubeRendererInstancedArrays(*renderer, getMainRenderTarget()->getRenderPass(), NUMBER_OF_TEXTURES, SCENE_RADIUS);
		}

		// Tell the cube renderer about the number of cubes
		if (nullptr != mCubeRenderer)
		{
			mCubeRenderer->setNumberOfCubes(mNumberOfCubeInstances);
		}
	}
}

void InstancedCubes::onDeinitialization()
{
	// Destroy the cube renderer, in case there's one
	if (nullptr != mCubeRenderer)
	{
		delete mCubeRenderer;
		mCubeRenderer = nullptr;
	}
}

void InstancedCubes::onUpdate()
{
	// Stop the stopwatch
	#ifdef RENDERER_RUNTIME
		mStopwatch.stop();
	#endif

	// Update the global timer (FPS independent movement)
	#ifdef RENDERER_RUNTIME
		const float timeDifference = mStopwatch.getMilliseconds();
	#else
		const float timeDifference = 0.0f;
	#endif
	mGlobalTimer += timeDifference;

	// Calculate the current FPS
	++mFramesSinceCheck;
	mFPSUpdateTimer += timeDifference;
	if (mFPSUpdateTimer > 1000.0f)
	{
		mFramesPerSecond   = static_cast<float>(mFramesSinceCheck / (mFPSUpdateTimer / 1000.0f));
		mFPSUpdateTimer   -= 1000.0f;
		mFramesSinceCheck  = 0;
	}

	// Start the stopwatch
	#ifdef RENDERER_RUNTIME
		mStopwatch.start();
	#endif

	{ // Input
		const DeviceInput::Keyboard* keyboard = mInputManager->GetKeyboard();
		if (keyboard->HasChanged())
		{
			// Add a fixed number of cubes
			if (keyboard->NumpadAdd.isHit() || keyboard->Add.isHit())
			{
				// Upper limit, just in case someone tries something nasty
				if (mNumberOfCubeInstances < UINT_MAX - NUMBER_OF_CHANGED_CUBES)
				{
					// Update the number of cubes
					mNumberOfCubeInstances += NUMBER_OF_CHANGED_CUBES;

					// Tell the cube renderer about the number of cubes
					if (nullptr != mCubeRenderer)
					{
						mCubeRenderer->setNumberOfCubes(mNumberOfCubeInstances);
					}
				}
			}

			// Subtract a fixed number of cubes
			if (keyboard->NumpadSubtract.isHit() || keyboard->Subtract.isHit())
			{
				// Lower limit
				if (mNumberOfCubeInstances > 1)
				{
					// Update the number of cubes
					if (mNumberOfCubeInstances > NUMBER_OF_CHANGED_CUBES)
					{
						mNumberOfCubeInstances -= NUMBER_OF_CHANGED_CUBES;
					}
					else
					{
						mNumberOfCubeInstances = 1;
					}

					// Tell the cube renderer about the number of cubes
					if (nullptr != mCubeRenderer)
					{
						mCubeRenderer->setNumberOfCubes(mNumberOfCubeInstances);
					}
				}
			}

			// Scale cubes up (change the size of all cubes at the same time)
			if (keyboard->Up.isHit())
			{
				mGlobalScale += 0.1f;
			}

			// Scale cubes down (change the size of all cubes at the same time)
			if (keyboard->Down.isHit())
			{
				mGlobalScale -= 0.1f;	// No need to check for negative values, results in entertaining inversed backface culling
			}

			// Show/hide statistics
			if (keyboard->Space.isHit())
			{
				// Toggle display of statistics
				mDisplayStatistics = !mDisplayStatistics;
			}
		}
		mInputManager->Update();
	}
}

void InstancedCubes::onDraw()
{
	// Get and check the renderer instance
	Renderer::IRendererPtr renderer(getRenderer());
	if (nullptr != renderer)
	{
		// Clear the color buffer of the current render target with gray, do also clear the depth buffer
		Renderer::Command::Clear::create(mCommandBuffer, Renderer::ClearFlag::COLOR_DEPTH, Color4::GRAY);

		// Draw the cubes
		if (nullptr != mCubeRenderer)
		{
			mCubeRenderer->fillCommandBuffer(mGlobalTimer, mGlobalScale, sin(mGlobalTimer * 0.001f) * SCENE_RADIUS, sin(mGlobalTimer * 0.0005f) * SCENE_RADIUS, cos(mGlobalTimer * 0.0008f) * SCENE_RADIUS, mCommandBuffer);
		}

		// Display statistics
		#ifdef RENDERER_RUNTIME_IMGUI
			if (mDisplayStatistics)
			{
				RendererRuntime::IRendererRuntime* rendererRuntime = getRendererRuntime();
				if (nullptr != rendererRuntime && nullptr != getMainRenderTarget())
				{
					RendererRuntime::DebugGuiManager& debugGuiManager = rendererRuntime->getDebugGuiManager();
					debugGuiManager.newFrame(*getMainRenderTarget());

					// Is there a cube renderer instance?
					if (nullptr != mCubeRenderer)
					{
						char text[128];

						// Number of cubes
						snprintf(text, glm::countof(text), "Number of cubes: %d", mNumberOfCubeInstances);
						RendererRuntime::DebugGuiHelper::drawText(text, 10.0f, 10.0f);

						// Frames per second
						snprintf(text, glm::countof(text), "Frames per second: %.2f", mFramesPerSecond);
						RendererRuntime::DebugGuiHelper::drawText(text, 10.0f, 40.0f);

						// Cubes per second
						// -> In every frame we draw n-cubes...
						// -> TODO(co) This number can get huge... had over 1 million cubes with >25 FPS... million cubes at ~2.4 FPS...
						snprintf(text, glm::countof(text), "Cubes per second: %u", static_cast<uint32_t>(mFramesPerSecond) * mNumberOfCubeInstances);
						RendererRuntime::DebugGuiHelper::drawText(text, 10.0f, 70.0f);
					}
					else
					{
						RendererRuntime::DebugGuiHelper::drawText("No cube renderer instance", 10.0f, 10.0f);
					}
					debugGuiManager.fillCommandBufferUsingFixedBuildInRendererConfiguration(mCommandBuffer);
				}
			}
		#endif

		// Submit command buffer to the renderer backend
		mCommandBuffer.submitToRendererAndClear(*renderer);
	}
}
