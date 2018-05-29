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
//[ Definitions                                           ]
//[-------------------------------------------------------]
// TODO(co) "ExampleBase.h" and "IApplicationRenderer.h" use the same definitions
#ifdef RENDERER_ONLY_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif defined(RENDERER_ONLY_OPENGL)
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif defined(RENDERER_ONLY_OPENGLES3)
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif defined(RENDERER_ONLY_VULKAN)
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif defined(RENDERER_ONLY_DIRECT3D9)
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif defined(RENDERER_ONLY_DIRECT3D10)
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D11
	#define RENDERER_NO_DIRECT3D12
#elif defined(RENDERER_ONLY_DIRECT3D11)
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D12
#elif defined(RENDERER_ONLY_DIRECT3D12)
	#define RENDERER_NO_NULL
	#define RENDERER_NO_OPENGL
	#define RENDERER_NO_OPENGLES3
	#define RENDERER_NO_VULKAN
	#define RENDERER_NO_DIRECT3D9
	#define RENDERER_NO_DIRECT3D10
	#define RENDERER_NO_DIRECT3D11
#endif


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class ILog;
	class IRenderer;
	class IRenderTarget;
}
namespace RendererRuntime
{
	class IRendererRuntime;
}
namespace RendererToolkit
{
	class IRendererToolkit;
}
class IApplicationFrontend;


//[-------------------------------------------------------]
//[ Classes                                               ]
//[-------------------------------------------------------]
/**
*  @brief
*    Example base class
*/
class ExampleBase
{


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Destructor
	*/
	inline virtual ~ExampleBase()
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Return the custom log instance
	*
	*  @return
	*    Custom log instance, can be a null pointer, don't destroy the instance
	*/
	inline Renderer::ILog* getCustomLog() const
	{
		return mCustomLog;
	}

	/**
	*  @brief
	*    Initializes the example; does nothing when already initialized
	*/
	inline void initialize()
	{
		if (!mInitialized)
		{
			onInitialization();
			mInitialized = true;
		}
	}

	/**
	*  @brief
	*    Deinitialize the example; does nothing when already deinitialized
	*/
	inline void deinitialize()
	{
		if (mInitialized)
		{
			onDeinitialization();
			mInitialized = false;
		}
	}

	/**
	*  @brief
	*    Let the example draw one frame
	*/
	inline void draw()
	{
		onDraw();
	}

	/**
	*  @brief
	*    Set the application frontend to be used by the example
	*/
	inline void setApplicationFrontend(IApplicationFrontend* applicationFrontend)
	{
		mApplicationFrontend = applicationFrontend;
	}

	/**
	*  @brief
	*    Return the renderer instance
	*
	*  @return
	*    The renderer instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
	*/
	Renderer::IRenderer* getRenderer() const;

	/**
	*  @brief
	*    Return the main renderer target
	*
	*  @return
	*    The main renderer target instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
	*/
	Renderer::IRenderTarget* getMainRenderTarget() const;

	/**
	*  @brief
	*    Return the renderer runtime instance
	*
	*  @return
	*    The renderer runtime instance, can be a null pointer
	*/
	RendererRuntime::IRendererRuntime* getRendererRuntime() const;

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
	RendererToolkit::IRendererToolkit* getRendererToolkit();


//[-------------------------------------------------------]
//[ Public virtual ExampleBase methods                    ]
//[-------------------------------------------------------]
public:
	inline virtual void onInitialization()
	{
		// Base does nothing
	}

	inline virtual void onDeinitialization()
	{
		// Base does nothing
	}

	inline virtual void onUpdate()
	{
		// Base does nothing
	}

	inline virtual void onDraw()
	{
		// Base does nothing
	}

	/**
	*  @brief
	*    Return if the examples does the drawing completely on its own; thus no draw handling in frontend (aka draw request handling in IApplicationRenderer)
	*
	*  @return
	*    "true" if the example does it's complete draw handling, otherwise "false"
	*/
	inline virtual bool doesCompleteOwnDrawing() const
	{
		// Default implementation does not complete own drawing
		return false;
	}


//[-------------------------------------------------------]
//[ Protected methods                                     ]
//[-------------------------------------------------------]
protected:
	/**
	*  @brief
	*    Default constructor
	*/
	inline ExampleBase() :
		mCustomLog(nullptr),
		mInitialized(false),
		mApplicationFrontend(nullptr)
	{
		// Nothing here
	}

	/**
	*  @brief
	*    Set custom log instance
	*
	*  @param[in] customLog
	*    Optional custom log instance, can be a null pointer, the instance must be valid as long as the example base instance exists
	*/
	inline void setCustomLog(Renderer::ILog* customLog = nullptr)
	{
		mCustomLog = customLog;
	}


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	Renderer::ILog*		  mCustomLog;			///< Optional custom log instance, can be a null pointer, don't destroy the instance
	bool				  mInitialized;
	IApplicationFrontend* mApplicationFrontend;	///< Renderer instance, can be a null pointer, do not destroy the instance


};
