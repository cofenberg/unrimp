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
#include "Framework/ExampleBase.h"

#ifdef RENDERER_RUNTIME
	#include <RendererRuntime/Core/Time/Stopwatch.h>
#endif


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace DeviceInput
{
	class InputManager;
}
class ICubeRenderer;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Cube instancing application class
*
*  @remarks
*    Demonstrates:
*    - Vertex buffer object (VBO)
*    - Vertex array object (VAO)
*    - Index buffer object (IBO)
*    - Uniform buffer object (UBO)
*    - Texture buffer object (TBO)
*    - 2D texture
*    - 2D texture array
*    - Sampler state object
*    - Vertex shader (VS) and fragment shader (FS)
*    - Root signature
*    - Graphics pipeline state object (PSO)
*    - Instanced arrays (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
*    - Draw instanced (shader model 4 feature, build in shader variable holding the current instance ID)
*/
class InstancedCubes final : public ExampleBase
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*/
	InstancedCubes();

	/**
	*  @brief
	*    Destructor
	*/
	virtual ~InstancedCubes() override;


//[-------------------------------------------------------]
//[ Public virtual IApplication methods                   ]
//[-------------------------------------------------------]
public:
	virtual void onInitialization() override;
	virtual void onDeinitialization() override;
	virtual void onUpdate() override;
	virtual void onDraw() override;


//[-------------------------------------------------------]
//[ Private static data                                   ]
//[-------------------------------------------------------]
private:
	static constexpr uint32_t NUMBER_OF_CHANGED_CUBES = 10000;	///< Number of changed cubes on key interaction
	static constexpr uint32_t NUMBER_OF_TEXTURES	  = 8;		///< Number of textures
	static constexpr uint32_t SCENE_RADIUS			  = 10;		///< Scene radius


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	DeviceInput::InputManager*	mInputManager;			///< Input manager instance, always valid
	ICubeRenderer*				mCubeRenderer;			///< Cube renderer instance, can be a null pointer
	uint32_t					mNumberOfCubeInstances;	///< Number of cube instances
	Renderer::CommandBuffer 	mCommandBuffer;			///< Command buffer
	// The rest is for timing and statistics
	#ifdef RENDERER_RUNTIME
		RendererRuntime::Stopwatch mStopwatch;	///< Stopwatch instance
	#endif
	float mGlobalTimer;			///< Global timer
	float mGlobalScale;			///< Global scale
	bool  mDisplayStatistics;	///< Display statistics?
	float mFPSUpdateTimer;		///< Timer for FPS update
	int   mFramesSinceCheck;	///< Number of frames since last FPS update
	float  mFramesPerSecond;	///< Current frames per second


};
