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
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class CameraSceneItem;
}


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Abstract controller interface
*
*  @note
*    - Remember: Unrimp is only about rendering and not about being a fully featured game engine, so just super basic stuff in here
*/
class IController
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~IController()
	{
		// Nothing here
	}

	[[nodiscard]] inline RendererRuntime::CameraSceneItem& getCameraSceneItem()
	{
		return mCameraSceneItem;
	}

	/**
	*  @brief
	*    Return whether or not mouse control is currently actively used (e.g. for looking around)
	*
	*  @return
	*    "true" if the mouse control is currently actively used (e.g. for looking around), else "false"
	*
	*  @note
	*    - This can be used to avoid that while looking around with the mouse the mouse is becoming considered hovering over an GUI element
	*/
	[[nodiscard]] inline bool isMouseControlInProgress() const
	{
		return mMouseControlInProgress;
	}


//[-------------------------------------------------------]
//[ Public virtual IController methods                    ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Called on update request
	*
	*  @param[in] pastSecondsSinceLastFrame
	*    Past seconds since last frame
	*  @param[in] inputEnabled
	*    "true" if input is enabled, else "false"
	*/
	virtual void onUpdate(float pastSecondsSinceLastFrame, bool inputEnabled) = 0;


//[-------------------------------------------------------]
//[ Protected methods                                     ]
//[-------------------------------------------------------]
protected:
	/**
	*  @brief
	*    Constructor
	*
	*  @param[in] cameraSceneItem
	*    Camera scene item to control, instance must stay valid as long as this controller instance exists
	*/
	inline explicit IController(RendererRuntime::CameraSceneItem& cameraSceneItem) :
		mCameraSceneItem(cameraSceneItem),
		mMouseControlInProgress(false)
	{
		// Nothing here
	}

	explicit IController(const IController&) = delete;
	IController& operator=(const IController&) = delete;


//[-------------------------------------------------------]
//[ Protected data                                        ]
//[-------------------------------------------------------]
protected:
	RendererRuntime::CameraSceneItem& mCameraSceneItem;
	bool							  mMouseControlInProgress;


};
