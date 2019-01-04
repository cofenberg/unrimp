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
#include <openvr/openvr.h>


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class SceneNode;
	class SceneResource;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract OpenVR manager listener interface
	*/
	class IVrManagerOpenVRListener
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class VrManagerOpenVR;	// Is calling the protected interface methods


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline IVrManagerOpenVRListener()
		{
			// Nothing here
		}

		inline virtual ~IVrManagerOpenVRListener()
		{
			// Nothing here
		}

		explicit IVrManagerOpenVRListener(const IVrManagerOpenVRListener&) = delete;
		IVrManagerOpenVRListener& operator=(const IVrManagerOpenVRListener&) = delete;


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::IVrManagerOpenVRListener methods ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void onVrEvent([[maybe_unused]] const vr::VREvent_t& vrVrEvent)
		{
			// Nothing here
		}

		inline virtual void onSceneNodeCreated([[maybe_unused]] vr::TrackedDeviceIndex_t trackedDeviceIndex, [[maybe_unused]] SceneResource& sceneResource, [[maybe_unused]] SceneNode& sceneNode)
		{
			// Nothing here
		}


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
