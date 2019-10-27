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
#include "RendererRuntime/Public/Resource/ShaderBlueprint/GraphicsShaderType.h"
#include "RendererRuntime/Public/Core/GetInvalid.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4623)	// warning C4623: 'std::_UInt_is_zero': default constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4625)	// warning C4625: 'std::codecvt_base': copy constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4626)	// warning C4626: 'std::codecvt<char16_t,char,_Mbstatet>': assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	PRAGMA_WARNING_DISABLE_MSVC(4774)	// warning C4774: 'sprintf_s' : format string expected in argument 3 is not a string literal
	PRAGMA_WARNING_DISABLE_MSVC(5026)	// warning C5026: 'std::_Generic_error_category': move constructor was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5027)	// warning C5027: 'std::_Generic_error_category': move assignment operator was implicitly defined as deleted
	PRAGMA_WARNING_DISABLE_MSVC(5039)	// warning C5039: '_Thrd_start': pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
	#include <vector>
	#include <string>
	#include <atomic>	// For "std::atomic<>"
	#include <deque>
	#include <mutex>
	#include <thread>
	#include <unordered_set>
	#include <condition_variable>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Rhi
{
	class IGraphicsProgram;
	class IGraphicsPipelineState;
}
namespace RendererRuntime
{
	class ShaderCache;
	class IRendererRuntime;
	class MaterialBlueprintResource;
	class GraphicsPipelineStateCache;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef uint32_t GraphicsProgramCacheId;	///< Graphics program cache identifier, result of hashing the shader combination IDs of the referenced shaders


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Graphics pipeline state compiler class
	*
	*  @remarks
	*    A graphics pipeline state must master the following stages in order to archive the inner wisdom:
	*    1. Asynchronous shader building
	*    2. Asynchronous shader compilation
	*    3. Synchronous RHI implementation dispatch TODO(co) Asynchronous RHI implementation dispatch if supported by the RHI implementation
	*
	*  @note
	*    - Takes care of asynchronous graphics pipeline state compilation
	*/
	class GraphicsPipelineStateCompiler final
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class RendererRuntimeImpl;
		friend class GraphicsPipelineStateCacheManager;	// Only the graphics pipeline state cache manager is allowed to commit compiler requests


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline bool isAsynchronousCompilationEnabled() const
		{
			return mAsynchronousCompilationEnabled;
		}

		void setAsynchronousCompilationEnabled(bool enabled);

		[[nodiscard]] inline uint32_t getNumberOfCompilerThreads() const
		{
			return mNumberOfCompilerThreads;
		}

		void setNumberOfCompilerThreads(uint32_t numberOfCompilerThreads);

		[[nodiscard]] inline uint32_t getNumberOfInFlightCompilerRequests() const
		{
			return mNumberOfInFlightCompilerRequests;
		}

		inline void flushBuilderQueue()
		{
			flushQueue(mBuilderMutex, mBuilderQueue);
		}

		inline void flushCompilerQueue()
		{
			flushQueue(mCompilerMutex, mCompilerQueue);
		}

		inline void flushAllQueues()
		{
			flushBuilderQueue();
			flushCompilerQueue();
		}

		void dispatch();


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		struct CompilerRequest final
		{
			// Input
			GraphicsPipelineStateCache&	 graphicsPipelineStateCache;
			// Internal
			GraphicsProgramCacheId		 graphicsProgramCacheId;
			ShaderCache*				 shaderCache[NUMBER_OF_GRAPHICS_SHADER_TYPES];
			std::string					 shaderSourceCode[NUMBER_OF_GRAPHICS_SHADER_TYPES];
			Rhi::IGraphicsPipelineState* graphicsPipelineStateObject;

			inline explicit CompilerRequest(GraphicsPipelineStateCache& _graphicsPipelineStateCache) :
				graphicsPipelineStateCache(_graphicsPipelineStateCache),
				graphicsProgramCacheId(getInvalid<GraphicsProgramCacheId>()),
				graphicsPipelineStateObject(nullptr)
			{
				for (uint8_t i = 0; i < NUMBER_OF_GRAPHICS_SHADER_TYPES; ++i)
				{
					shaderCache[i] = nullptr;
				}
			}
			inline explicit CompilerRequest(const CompilerRequest& compilerRequest) :
				graphicsPipelineStateCache(compilerRequest.graphicsPipelineStateCache),
				graphicsProgramCacheId(compilerRequest.graphicsProgramCacheId),
				graphicsPipelineStateObject(compilerRequest.graphicsPipelineStateObject)
			{
				for (uint8_t i = 0; i < NUMBER_OF_GRAPHICS_SHADER_TYPES; ++i)
				{
					shaderCache[i]		= compilerRequest.shaderCache[i];
					shaderSourceCode[i] = compilerRequest.shaderSourceCode[i];
				}
			}
			CompilerRequest& operator=(const CompilerRequest&) = delete;
		};

		typedef std::vector<std::thread> CompilerThreads;
		typedef std::deque<CompilerRequest> CompilerRequests;
		typedef std::unordered_set<GraphicsProgramCacheId> InFlightGraphicsProgramCaches;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsPipelineStateCompiler(IRendererRuntime& rendererRuntime);
		explicit GraphicsPipelineStateCompiler(const GraphicsPipelineStateCompiler&) = delete;
		~GraphicsPipelineStateCompiler();
		GraphicsPipelineStateCompiler& operator=(const GraphicsPipelineStateCompiler&) = delete;
		void addAsynchronousCompilerRequest(GraphicsPipelineStateCache& graphicsPipelineStateCache);
		void instantSynchronousCompilerRequest(MaterialBlueprintResource& materialBlueprintResource, GraphicsPipelineStateCache& graphicsPipelineStateCache);
		void flushQueue(std::mutex& mutex, const CompilerRequests& compilerRequests);
		void builderThreadWorker();
		void compilerThreadWorker();
		[[nodiscard]] Rhi::IGraphicsPipelineState* createGraphicsPipelineState(const RendererRuntime::MaterialBlueprintResource& materialBlueprintResource, uint32_t serializedGraphicsPipelineStateHash, Rhi::IGraphicsProgram& graphicsProgram) const;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		IRendererRuntime&			  mRendererRuntime;	///< Renderer runtime instance, do not destroy the instance
		bool						  mAsynchronousCompilationEnabled;
		uint32_t					  mNumberOfCompilerThreads;
		std::atomic<uint32_t>		  mNumberOfInFlightCompilerRequests;
		std::mutex					  mInFlightGraphicsProgramCachesMutex;
		InFlightGraphicsProgramCaches mInFlightGraphicsProgramCaches;

		// Asynchronous building (moderate cost)
		std::atomic<bool>		mShutdownBuilderThread;
		std::mutex				mBuilderMutex;
		std::condition_variable	mBuilderConditionVariable;
		CompilerRequests		mBuilderQueue;
		std::thread				mBuilderThread;

		// Asynchronous compilation (nuts cost)
		std::atomic<bool>		mShutdownCompilerThread;
		std::mutex				mCompilerMutex;
		std::condition_variable	mCompilerConditionVariable;
		CompilerRequests		mCompilerQueue;
		CompilerThreads			mCompilerThreads;

		// Synchronous dispatch
		std::mutex		 mDispatchMutex;
		CompilerRequests mDispatchQueue;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
