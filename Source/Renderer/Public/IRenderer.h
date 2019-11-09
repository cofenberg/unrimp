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
#include "Renderer/Public/Core/StringId.h"

#include <Rhi/Public/Rhi.h>

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::_Func_impl_no_alloc<Concurrency::details::_Task_impl<_ReturnType>::_CancelAndRunContinuations::<lambda_0456396a71e3abd88ede77bdd2823d8e>,_Ret>': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::_Func_impl_no_alloc<Concurrency::details::_Task_impl<_ReturnType>::_CancelAndRunContinuations::<lambda_0456396a71e3abd88ede77bdd2823d8e>,_Ret>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Func_impl_no_alloc<Concurrency::details::_Task_impl<_ReturnType>::_CancelAndRunContinuations::<lambda_0456396a71e3abd88ede77bdd2823d8e>,_Ret>': move assignment operator was implicitly defined as deleted
	#include <vector>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Renderer
{
	class Context;
	class TimeManager;
	class IFileManager;
	class AssetManager;
	class IRenderer;
	class ResourceStreamer;
	class IResourceManager;
	class MeshResourceManager;
	class SceneResourceManager;
	class TextureResourceManager;
	class MaterialResourceManager;
	class SkeletonResourceManager;
	class RendererResourceManager;
	class ShaderPieceResourceManager;
	class ComputePipelineStateCompiler;
	class GraphicsPipelineStateCompiler;
	class CompositorNodeResourceManager;
	class ShaderBlueprintResourceManager;
	class VertexAttributesResourceManager;
	class SkeletonAnimationResourceManager;
	class MaterialBlueprintResourceManager;
	class CompositorWorkspaceResourceManager;
	template <typename ReturnType> class ThreadPool;
	typedef ThreadPool<void> DefaultThreadPool;
	#ifdef RENDERER_IMGUI
		class DebugGuiManager;
	#endif
	#ifdef RENDERER_OPENVR
		class IVrManager;
	#endif
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace Renderer
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;	///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset directory>/<asset name>"


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Abstract renderer interface
	*/
	class IRenderer : public Rhi::RefCount<IRenderer>
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<IResourceManager*> ResourceManagers;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IRenderer() override
		{
			// Nothing here
		}

		//[-------------------------------------------------------]
		//[ Core                                                  ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Return the used renderer context instance
		*
		*  @return
		*    The used renderer context instance
		*/
		[[nodiscard]] inline Context& getContext() const
		{
			return mContext;
		}

		/**
		*  @brief
		*    Return the used RHI instance
		*
		*  @return
		*    The used RHI instance, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline Rhi::IRhi& getRhi() const
		{
			return *mRhi;
		}

		/**
		*  @brief
		*    Return the used buffer manager instance
		*
		*  @return
		*    The used buffer manager instance, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline Rhi::IBufferManager& getBufferManager() const
		{
			return *mBufferManager;
		}

		/**
		*  @brief
		*    Return the used texture manager instance
		*
		*  @return
		*    The used texture manager instance, do not release the returned instance unless you added an own reference to it
		*/
		[[nodiscard]] inline Rhi::ITextureManager& getTextureManager() const
		{
			return *mTextureManager;
		}

		/**
		*  @brief
		*    Return the file manager instance
		*
		*  @return
		*    The file manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline IFileManager& getFileManager() const
		{
			return *mFileManager;
		}

		/**
		*  @brief
		*    Return the default thread pool instance
		*
		*  @return
		*    The default thread pool instance, do not release the returned instance
		*/
		[[nodiscard]] inline DefaultThreadPool& getDefaultThreadPool() const
		{
			return *mDefaultThreadPool;
		}

		/**
		*  @brief
		*    Return the asset manager instance
		*
		*  @return
		*    The asset manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline AssetManager& getAssetManager() const
		{
			return *mAssetManager;
		}

		/**
		*  @brief
		*    Return the time manager instance
		*
		*  @return
		*    The time manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline TimeManager& getTimeManager() const
		{
			return *mTimeManager;
		}

		//[-------------------------------------------------------]
		//[ Resource                                              ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Return the renderer resource manager instance
		*
		*  @return
		*    The renderer resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline RendererResourceManager& getRendererResourceManager() const
		{
			return *mRendererResourceManager;
		}

		/**
		*  @brief
		*    Return the resource streamer instance
		*
		*  @return
		*    The resource streamer instance, do not release the returned instance
		*/
		[[nodiscard]] inline ResourceStreamer& getResourceStreamer() const
		{
			return *mResourceStreamer;
		}

		/**
		*  @brief
		*    Return the vertex attributes resource manager instance
		*
		*  @return
		*    The vertex attributes resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline VertexAttributesResourceManager& getVertexAttributesResourceManager() const
		{
			return *mVertexAttributesResourceManager;
		}

		/**
		*  @brief
		*    Return the texture resource manager instance
		*
		*  @return
		*    The texture resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline TextureResourceManager& getTextureResourceManager() const
		{
			return *mTextureResourceManager;
		}

		/**
		*  @brief
		*    Return the shader piece resource manager instance
		*
		*  @return
		*    The shader piece resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline ShaderPieceResourceManager& getShaderPieceResourceManager() const
		{
			return *mShaderPieceResourceManager;
		}

		/**
		*  @brief
		*    Return the shader blueprint resource manager instance
		*
		*  @return
		*    The shader blueprint resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline ShaderBlueprintResourceManager& getShaderBlueprintResourceManager() const
		{
			return *mShaderBlueprintResourceManager;
		}

		/**
		*  @brief
		*    Return the material blueprint resource manager instance
		*
		*  @return
		*    The material blueprint resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline MaterialBlueprintResourceManager& getMaterialBlueprintResourceManager() const
		{
			return *mMaterialBlueprintResourceManager;
		}

		/**
		*  @brief
		*    Return the material resource manager instance
		*
		*  @return
		*    The material resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline MaterialResourceManager& getMaterialResourceManager() const
		{
			return *mMaterialResourceManager;
		}

		/**
		*  @brief
		*    Return the skeleton resource manager instance
		*
		*  @return
		*    The skeleton resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline SkeletonResourceManager& getSkeletonResourceManager() const
		{
			return *mSkeletonResourceManager;
		}

		/**
		*  @brief
		*    Return the skeleton animation resource manager instance
		*
		*  @return
		*    The skeleton animation resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline SkeletonAnimationResourceManager& getSkeletonAnimationResourceManager() const
		{
			return *mSkeletonAnimationResourceManager;
		}

		/**
		*  @brief
		*    Return the mesh resource manager instance
		*
		*  @return
		*    The mesh resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline MeshResourceManager& getMeshResourceManager() const
		{
			return *mMeshResourceManager;
		}

		/**
		*  @brief
		*    Return the scene resource manager instance
		*
		*  @return
		*    The scene resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline SceneResourceManager& getSceneResourceManager() const
		{
			return *mSceneResourceManager;
		}

		/**
		*  @brief
		*    Return the compositor node resource manager instance
		*
		*  @return
		*    The compositor node resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline CompositorNodeResourceManager& getCompositorNodeResourceManager() const
		{
			return *mCompositorNodeResourceManager;
		}

		/**
		*  @brief
		*    Return the compositor workspace resource manager instance
		*
		*  @return
		*    The compositor workspace resource manager instance, do not release the returned instance
		*/
		[[nodiscard]] inline CompositorWorkspaceResourceManager& getCompositorWorkspaceResourceManager() const
		{
			return *mCompositorWorkspaceResourceManager;
		}

		/**
		*  @brief
		*    Return a list of all resource manager instances
		*
		*  @return
		*    List of all resource manager instances, do not release the returned instances
		*/
		[[nodiscard]] inline const ResourceManagers& getResourceManagers() const
		{
			return mResourceManagers;
		}

		//[-------------------------------------------------------]
		//[ Misc                                                  ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Return the graphics pipeline state compiler instance
		*
		*  @return
		*    The graphics pipeline state compiler instance, do not release the returned instance
		*/
		[[nodiscard]] inline GraphicsPipelineStateCompiler& getGraphicsPipelineStateCompiler() const
		{
			return *mGraphicsPipelineStateCompiler;
		}

		/**
		*  @brief
		*    Return the compute pipeline state compiler instance
		*
		*  @return
		*    The compute pipeline state compiler instance, do not release the returned instance
		*/
		[[nodiscard]] inline ComputePipelineStateCompiler& getComputePipelineStateCompiler() const
		{
			return *mComputePipelineStateCompiler;
		}

		//[-------------------------------------------------------]
		//[ Optional                                              ]
		//[-------------------------------------------------------]
		#ifdef RENDERER_IMGUI
			/**
			*  @brief
			*    Return the debug GUI manager instance
			*
			*  @return
			*    The debug GUI manager instance, do not release the returned instance
			*/
			[[nodiscard]] inline DebugGuiManager& getDebugGuiManager() const
			{
				return *mDebugGuiManager;
			}
		#endif

		#ifdef RENDERER_OPENVR
			/**
			*  @brief
			*    Return the VR manager instance
			*
			*  @return
			*    The VR manager instance, do not release the returned instance
			*/
			[[nodiscard]] inline IVrManager& getVrManager() const
			{
				return *mVrManager;
			}
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Reload resource by using the given asset
		*
		*  @param[in] assetId
		*    ID of the asset which has been changed and hence the according resource needs to be reloaded
		*
		*  @note
		*    - This method is most likely called by a background thread
		*/
		virtual void reloadResourceByAssetId(AssetId assetId) = 0;

		/**
		*  @brief
		*    Flush all queues
		*/
		virtual void flushAllQueues() = 0;

		/**
		*  @brief
		*    Renderer update
		*
		*  @note
		*    - Call this once per frame
		*/
		virtual void update() = 0;

		//[-------------------------------------------------------]
		//[ Pipeline state object cache                           ]
		//[-------------------------------------------------------]
		virtual void clearPipelineStateObjectCache() = 0;
		virtual void loadPipelineStateObjectCache() = 0;
		virtual void savePipelineStateObjectCache() = 0;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] context
		*    Renderer context, the renderer context instance must stay valid as long as the renderer instance exists
		*/
		inline explicit IRenderer(Context& context) :
			// Core
			mContext(context),
			mRhi(nullptr),
			mBufferManager(nullptr),
			mTextureManager(nullptr),
			mFileManager(nullptr),
			mDefaultThreadPool(nullptr),
			mAssetManager(nullptr),
			mTimeManager(nullptr),
			// Resource
			mRendererResourceManager(nullptr),
			mResourceStreamer(nullptr),
			mVertexAttributesResourceManager(nullptr),
			mTextureResourceManager(nullptr),
			mShaderPieceResourceManager(nullptr),
			mShaderBlueprintResourceManager(nullptr),
			mMaterialBlueprintResourceManager(nullptr),
			mMaterialResourceManager(nullptr),
			mSkeletonResourceManager(nullptr),
			mSkeletonAnimationResourceManager(nullptr),
			mMeshResourceManager(nullptr),
			mSceneResourceManager(nullptr),
			mCompositorNodeResourceManager(nullptr),
			mCompositorWorkspaceResourceManager(nullptr),
			// Misc
			mGraphicsPipelineStateCompiler(nullptr),
			mComputePipelineStateCompiler(nullptr)
			// Optional
			#ifdef RENDERER_IMGUI
				, mDebugGuiManager(nullptr)
			#endif
			#ifdef RENDERER_OPENVR
				, mVrManager(nullptr)
			#endif
		{
			// Nothing here
		}

		explicit IRenderer(const IRenderer& source) = delete;
		IRenderer& operator =(const IRenderer& source) = delete;


	//[-------------------------------------------------------]
	//[ Protected data                                        ]
	//[-------------------------------------------------------]
	protected:
		// Core
		Context&			  mContext;			///< Renderer context
		Rhi::IRhi*			  mRhi;				///< The used RHI instance (we keep a reference to it), always valid
		Rhi::IBufferManager*  mBufferManager;	///< The used RHI buffer manager instance (we keep a reference to it), always valid
		Rhi::ITextureManager* mTextureManager;	///< The used RHI texture manager instance (we keep a reference to it), always valid
		IFileManager*		  mFileManager;		///< The used file manager instance, always valid
		DefaultThreadPool*	  mDefaultThreadPool;
		AssetManager*		  mAssetManager;
		TimeManager*		  mTimeManager;
		// Resource
		RendererResourceManager*			mRendererResourceManager;
		ResourceStreamer*					mResourceStreamer;
		VertexAttributesResourceManager*	mVertexAttributesResourceManager;
		TextureResourceManager*				mTextureResourceManager;
		ShaderPieceResourceManager*			mShaderPieceResourceManager;
		ShaderBlueprintResourceManager*		mShaderBlueprintResourceManager;
		MaterialBlueprintResourceManager*	mMaterialBlueprintResourceManager;
		MaterialResourceManager*			mMaterialResourceManager;
		SkeletonResourceManager*			mSkeletonResourceManager;
		SkeletonAnimationResourceManager*	mSkeletonAnimationResourceManager;
		MeshResourceManager*				mMeshResourceManager;
		SceneResourceManager*				mSceneResourceManager;
		CompositorNodeResourceManager*		mCompositorNodeResourceManager;
		CompositorWorkspaceResourceManager*	mCompositorWorkspaceResourceManager;
		ResourceManagers					mResourceManagers;
		// Misc
		GraphicsPipelineStateCompiler* mGraphicsPipelineStateCompiler;
		ComputePipelineStateCompiler*  mComputePipelineStateCompiler;
		// Optional
		#ifdef RENDERER_IMGUI
			DebugGuiManager* mDebugGuiManager;
		#endif
		#ifdef RENDERER_OPENVR
			IVrManager* mVrManager;
		#endif


	};


	//[-------------------------------------------------------]
	//[ Type definitions                                      ]
	//[-------------------------------------------------------]
	typedef Rhi::SmartRefCount<IRenderer> IRendererPtr;


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // Renderer
