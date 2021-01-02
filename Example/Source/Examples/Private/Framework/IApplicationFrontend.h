/*********************************************************\
 * Copyright (c) 2012-2021 The Unrimp Team
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
#include "Examples/Private/Framework/PlatformTypes.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Rhi
{
	class IRhi;
	class IRenderTarget;
}
namespace Renderer
{
	class IRenderer;
}
namespace RendererToolkit
{
	class IRendererToolkit;
}


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Abstract application frontend interface
*/
class IApplicationFrontend
{


//[-------------------------------------------------------]
//[ Public virtual IApplicationFrontend methods           ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~IApplicationFrontend()
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Ask the application politely to switch to another example as soon as possible
	*
	*  @param[in] exampleName
	*    Example name, must be valid
	*  @param[in] rhiName
	*    RHI name, if null pointer the default RHI will be used
	*/
	virtual void switchExample(const char* exampleName, const char* rhiName = nullptr) = 0;

	/**
	*  @brief
	*    Ask the application politely to shut down as soon as possible
	*/
	virtual void exit() = 0;

	/**
	*  @brief
	*    Return the RHI instance
	*
	*  @return
	*    The RHI instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
	*/
	[[nodiscard]] virtual Rhi::IRhi* getRhi() const = 0;

	/**
	*  @brief
	*    Return the main RHI target
	*
	*  @return
	*    The main RHI target instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
	*/
	[[nodiscard]] virtual Rhi::IRenderTarget* getMainRenderTarget() const = 0;

	/**
	*  @brief
	*    Return the renderer instance
	*
	*  @return
	*    The renderer instance, can be a null pointer
	*/
	[[nodiscard]] inline virtual Renderer::IRenderer* getRenderer() const
	{
		// Base implementation returns always a null pointer
		return nullptr;
	}

	/**
	*  @brief
	*    Return the renderer toolkit instance
	*
	*  @return
	*    The renderer toolkit instance, can be a null pointer
	*
	*  @remarks
	*    During runtime, the renderer toolkit can optionally be used to enable asset hot-reloading. Meaning,
	*    as soon as an source asset gets changed, the asset is recompiled in a background thread and the compiled
	*    runtime-ready asset is reloaded. One can see the change in realtime without the need to restart the application.
	*
	*    This feature links during runtime the renderer toolkit as soon as this method is accessed the first time. If
	*    the renderer toolkit shared library is not there, this method will return a null pointer. This is a developer-feature
	*    and as such, it's not available in static builds which are meant for the end-user who e.g. just want to "play the game".
	*/
	[[nodiscard]] inline virtual RendererToolkit::IRendererToolkit* getRendererToolkit()
	{
		// Base implementation returns always a null pointer
		return nullptr;
	}


};
