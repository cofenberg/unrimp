/*********************************************************\
 * Copyright (c) 2012-2020 The Unrimp Team
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

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	#include <string_view>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Rhi
{
	class ILog;
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
class ExampleRunner;
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
//[ Friends                                               ]
//[-------------------------------------------------------]
friend class ExampleRunner;


//[-------------------------------------------------------]
//[ Public methods                                        ]
//[-------------------------------------------------------]
public:
	/**
	*  @brief
	*    Constructor
	*/
	inline explicit ExampleBase(ExampleRunner& exampleRunner) :
		mExampleRunner(&exampleRunner),
		mCustomLog(nullptr),
		mApplicationFrontend(nullptr)
	{
		// Nothing here
	}

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
	*    Return the example runner instance
	*
	*  @return
	*    The example runner instance
	*/
	[[nodiscard]] inline ExampleRunner& getExampleRunner()
	{
		ASSERT((nullptr != mExampleRunner) && "Don't call this method inside constructors");
		return *mExampleRunner;
	}

	/**
	*  @brief
	*    Return the example name
	*
	*  @return
	*    The example name
	*/
	[[nodiscard]] inline const std::string_view& getExampleName() const
	{
		return mExampleName;
	}

	/**
	*  @brief
	*    Return the custom log instance
	*
	*  @return
	*    Custom log instance, can be a null pointer, don't destroy the instance
	*/
	[[nodiscard]] inline Rhi::ILog* getCustomLog() const
	{
		return mCustomLog;
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
	*    Return the RHI instance
	*
	*  @return
	*    The RHI instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
	*/
	[[nodiscard]] Rhi::IRhi* getRhi() const;

	/**
	*  @brief
	*    Return the main RHI target
	*
	*  @return
	*    The main RHI target instance, can be a null pointer, do not release the returned instance unless you added an own reference to it
	*/
	[[nodiscard]] Rhi::IRenderTarget* getMainRenderTarget() const;

	/**
	*  @brief
	*    Return the renderer instance
	*
	*  @return
	*    The renderer instance, can be a null pointer
	*/
	[[nodiscard]] Renderer::IRenderer* getRenderer() const;

	/**
	*  @brief
	*    Return the renderer instance
	*
	*  @return
	*    The renderer instance
	*/
	[[nodiscard]] Renderer::IRenderer& getRendererSafe() const;

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
	[[nodiscard]] RendererToolkit::IRendererToolkit* getRendererToolkit();

	/**
	*  @brief
	*    Ask the application politely to switch to another example as soon as possible
	*
	*  @param[in] exampleName
	*    Example name, must be valid
	*  @param[in] rhiName
	*    RHI name, if null pointer the default RHI will be used
	*/
	void switchExample(const char* exampleName, const char* rhiName = nullptr);

	/**
	*  @brief
	*    Ask the application politely to shut down as soon as possible
	*/
	void exit();


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
	*    Return if the examples does the drawing completely on its own; thus no draw handling in frontend (aka draw request handling in "IApplicationRhi")
	*
	*  @return
	*    "true" if the example does its complete draw handling, otherwise "false"
	*/
	[[nodiscard]] inline virtual bool doesCompleteOwnDrawing() const
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
	*    Constructor
	*/
	inline ExampleBase() :
		mExampleRunner(nullptr),
		mCustomLog(nullptr),
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
	inline void setCustomLog(Rhi::ILog* customLog = nullptr)
	{
		mCustomLog = customLog;
	}


//[-------------------------------------------------------]
//[ Private data                                          ]
//[-------------------------------------------------------]
private:
	ExampleRunner*		  mExampleRunner;
	std::string_view	  mExampleName;
	Rhi::ILog*			  mCustomLog;			///< Optional custom log instance, can be a null pointer, don't destroy the instance
	IApplicationFrontend* mApplicationFrontend;	///< RHI instance, can be a null pointer, do not destroy the instance


};
