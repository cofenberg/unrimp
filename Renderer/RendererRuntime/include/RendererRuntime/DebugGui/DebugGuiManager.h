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
#include "RendererRuntime/Export.h"
#include "RendererRuntime/Core/Manager.h"
#include "RendererRuntime/Core/StringId.h"

#include <Renderer/Renderer.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	#include <string>
	#include <vector>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
struct ImGuiContext;
namespace RendererRuntime
{
	class IRendererRuntime;
	class CompositorWorkspaceInstance;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId			 AssetId;	///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset type>/<asset category>/<asset name>"
	typedef std::vector<AssetId> AssetIds;


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Debug GUI manager using ImGui ( https://github.com/ocornut/imgui )
	*
	*  @remarks
	*    Supports two command buffer fill modes:
	*    - Using fixed build in renderer configuration, including shaders
	*    - Using a material resource blueprint set by the caller
	*/
	class DebugGuiManager : private Manager
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RendererRuntimeImpl;


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the asset IDs of automatically generated dynamic default texture assets
		*
		*  @param[out] assetIds
		*    Receives the asset IDs of automatically generated dynamic default texture assets, the list is not cleared before new entries are added
		*
		*  @remarks
		*    The debug GUI manager automatically generates some dynamic default texture assets one can reference e.g. inside material blueprint resources:
		*    - "Unrimp/Texture/DynamicByCode/ImGuiGlyphMap2D"
		*/
		static void getDefaultTextureAssetIds(AssetIds& assetIds);


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		RENDERERRUNTIME_API_EXPORT void newFrame(Renderer::IRenderTarget& renderTarget, CompositorWorkspaceInstance* compositorWorkspaceInstance = nullptr);
		RENDERERRUNTIME_API_EXPORT Renderer::IVertexArrayPtr getFillVertexArrayPtr();
		RENDERERRUNTIME_API_EXPORT void fillCommandBuffer(Renderer::CommandBuffer& commandBuffer);
		RENDERERRUNTIME_API_EXPORT void fillCommandBufferUsingFixedBuildInRendererConfiguration(Renderer::CommandBuffer& commandBuffer);

		// Ini-settings
		RENDERERRUNTIME_API_EXPORT bool getIniSetting(const char* name, float value[4]);
		RENDERERRUNTIME_API_EXPORT void setIniSetting(const char* name, const float value[4]);

		// Helper
		inline void openMetricsWindow()
		{
			if (!mOpenMetricsWindow)
			{
				mOpenMetricsWindow = true;
				setOpenMetricsWindowIniSetting();
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual RendererRuntime::DebugGuiManager methods ]
	//[-------------------------------------------------------]
	protected:
		virtual void startup();
		virtual void onNewFrame(Renderer::IRenderTarget& renderTarget) = 0;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		explicit DebugGuiManager(IRendererRuntime& rendererRuntime);
		virtual ~DebugGuiManager();
		explicit DebugGuiManager(const DebugGuiManager&) = delete;
		DebugGuiManager& operator=(const DebugGuiManager&) = delete;
		void createFixedBuildInRendererConfigurationResources();
		RENDERERRUNTIME_API_EXPORT void setOpenMetricsWindowIniSetting();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime&			mRendererRuntime;		///< Renderer runtime instance, do not destroy the instance
		ImGuiContext*				mImGuiContext;			///< ImGui context, always valid
		std::string					mAbsoluteIniFilename;	///< Absolute UTF-8 ImGui ini-filename, class member since ImGui just keeps a pointer to this string instance
		std::string					mAbsoluteLogFilename;	///< Absolute UTF-8 ImGui log-filename, class member since ImGui just keeps a pointer to this string instance
		bool						mIsRunning;				///< The debug GUI manager will be initialized lazy when "RendererRuntime::DebugGuiManager::newFrame()" is called for the first time
		Renderer::ITexture2DPtr		mTexture2D;
		// Fixed build in renderer configuration resources
		Renderer::IRootSignaturePtr	mRootSignature;
		Renderer::IProgramPtr		mProgram;
		Renderer::IPipelineStatePtr	mPipelineState;
		Renderer::IUniformBufferPtr	mVertexShaderUniformBuffer;
		Renderer::handle			mObjectSpaceToClipSpaceMatrixUniformHandle;
		Renderer::IResourceGroupPtr	mResourceGroup;		///< Resource group, can be a null pointer
		Renderer::IResourceGroupPtr	mSamplerStateGroup;	///< Sampler state resource group, can be a null pointer
		// Vertex and index buffer
		Renderer::IVertexBufferPtr	mVertexBufferPtr;
		uint32_t					mNumberOfAllocatedVertices;
		Renderer::IIndexBufferPtr	mIndexBufferPtr;
		uint32_t					mNumberOfAllocatedIndices;
		Renderer::IVertexArrayPtr	mVertexArrayPtr;
		// Helper
		bool mOpenMetricsWindow;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
