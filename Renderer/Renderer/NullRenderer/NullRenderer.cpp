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


/**
*  @brief
*    Null renderer amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    None.
*
*    == Preprocessor Definitions ==
*    - Set "NULLRENDERER_EXPORTS" as preprocessor definition when building this library as shared library
*    - Do also have a look into the renderer header file documentation
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Renderer/Renderer.h>

#ifdef _WIN32
	// Set Windows version to Windows Vista (0x0600), we don't support Windows XP (0x0501)
	#ifdef WINVER
		#undef WINVER
	#endif
	#define WINVER			0x0600
	#ifdef _WIN32_WINNT
		#undef _WIN32_WINNT
	#endif
	#define _WIN32_WINNT	0x0600

	// Exclude some stuff from "windows.h" to speed up compilation a bit
	#define WIN32_LEAN_AND_MEAN
	#define NOGDICAPMASKS
	#define NOMENUS
	#define NOICONS
	#define NOKEYSTATES
	#define NOSYSCOMMANDS
	#define NORASTEROPS
	#define OEMRESOURCE
	#define NOATOM
	#define NOMEMMGR
	#define NOMETAFILE
	#define NOOPENFILE
	#define NOSCROLL
	#define NOSERVICE
	#define NOSOUND
	#define NOWH
	#define NOCOMM
	#define NOKANJI
	#define NOHELP
	#define NOPROFILER
	#define NODEFERWINDOWPOS
	#define NOMCX
	#define NOCRYPT
	#include <windows.h>
#elif LINUX
	// TODO(co) Review which of the following headers can be removed
	#include <X11/Xlib.h>

	#include <string.h>
#endif




//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace NullRenderer
{
	class RootSignature;
}




//[-------------------------------------------------------]
//[ Macros & definitions                                  ]
//[-------------------------------------------------------]
/*
*  @brief
*    Check whether or not the given resource is owned by the given renderer
*/
#ifdef RENDERER_DEBUG
	#define NULLRENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference) \
		RENDERER_ASSERT(mContext, &rendererReference == &(resourceReference).getRenderer(), "Null error: The given resource is owned by another renderer instance")
#else
	#define NULLRENDERER_RENDERERMATCHCHECK_ASSERT(rendererReference, resourceReference)
#endif




//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr const char* NULL_NAME = "Null";	///< ASCII name of this shader language, always valid (do not free the memory the returned pointer is pointing to)


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace NullRenderer
{




	//[-------------------------------------------------------]
	//[ NullRenderer/NullRenderer.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null renderer class
	*/
	class NullRenderer final : public Renderer::IRenderer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] context
		*    Renderer context, the renderer context instance must stay valid as long as the renderer instance exists
		*
		*  @note
		*    - Do never ever use a not properly initialized renderer! Use "Renderer::IRenderer::isInitialized()" to check the initialization state.
		*/
		explicit NullRenderer(const Renderer::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~NullRenderer() override;

		//[-------------------------------------------------------]
		//[ States                                                ]
		//[-------------------------------------------------------]
		void setGraphicsRootSignature(Renderer::IRootSignature* rootSignature);
		void setGraphicsResourceGroup(uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup);
		void setPipelineState(Renderer::IPipelineState* pipelineState);
		//[-------------------------------------------------------]
		//[ Input-assembler (IA) stage                            ]
		//[-------------------------------------------------------]
		void iaSetVertexArray(Renderer::IVertexArray* vertexArray);
		//[-------------------------------------------------------]
		//[ Rasterizer (RS) stage                                 ]
		//[-------------------------------------------------------]
		void rsSetViewports(uint32_t numberOfViewports, const Renderer::Viewport* viewports);
		void rsSetScissorRectangles(uint32_t numberOfScissorRectangles, const Renderer::ScissorRectangle* scissorRectangles);
		//[-------------------------------------------------------]
		//[ Output-merger (OM) stage                              ]
		//[-------------------------------------------------------]
		void omSetRenderTarget(Renderer::IRenderTarget* renderTarget);
		//[-------------------------------------------------------]
		//[ Operations                                            ]
		//[-------------------------------------------------------]
		void clear(uint32_t flags, const float color[4], float z, uint32_t stencil);
		void resolveMultisampleFramebuffer(Renderer::IRenderTarget& destinationRenderTarget, Renderer::IFramebuffer& sourceMultisampleFramebuffer);
		void copyResource(Renderer::IResource& destinationResource, Renderer::IResource& sourceResource);
		//[-------------------------------------------------------]
		//[ Draw call                                             ]
		//[-------------------------------------------------------]
		void drawEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RENDERER_DEBUG
			void setDebugMarker(const char* name);
			void beginDebugEvent(const char* name);
			void endDebugEvent();
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	public:
		virtual const char* getName() const override;
		virtual bool isInitialized() const override;
		virtual bool isDebugEnabled() override;
		//[-------------------------------------------------------]
		//[ Shader language                                       ]
		//[-------------------------------------------------------]
		virtual uint32_t getNumberOfShaderLanguages() const override;
		virtual const char* getShaderLanguageName(uint32_t index) const override;
		virtual Renderer::IShaderLanguage* getShaderLanguage(const char* shaderLanguageName = nullptr) override;
		//[-------------------------------------------------------]
		//[ Resource creation                                     ]
		//[-------------------------------------------------------]
		virtual Renderer::IRenderPass* createRenderPass(uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat = Renderer::TextureFormat::UNKNOWN, uint8_t numberOfMultisamples = 1) override;
		virtual Renderer::ISwapChain* createSwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, bool useExternalContext = false) override;
		virtual Renderer::IFramebuffer* createFramebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment = nullptr) override;
		virtual Renderer::IBufferManager* createBufferManager() override;
		virtual Renderer::ITextureManager* createTextureManager() override;
		virtual Renderer::IRootSignature* createRootSignature(const Renderer::RootSignature& rootSignature) override;
		virtual Renderer::IPipelineState* createPipelineState(const Renderer::PipelineState& pipelineState) override;
		virtual Renderer::ISamplerState* createSamplerState(const Renderer::SamplerState& samplerState) override;
		//[-------------------------------------------------------]
		//[ Resource handling                                     ]
		//[-------------------------------------------------------]
		virtual bool map(Renderer::IResource& resource, uint32_t subresource, Renderer::MapType mapType, uint32_t mapFlags, Renderer::MappedSubresource& mappedSubresource) override;
		virtual void unmap(Renderer::IResource& resource, uint32_t subresource) override;
		//[-------------------------------------------------------]
		//[ Operations                                            ]
		//[-------------------------------------------------------]
		virtual bool beginScene() override;
		virtual void submitCommandBuffer(const Renderer::CommandBuffer& commandBuffer) override;
		virtual void endScene() override;
		//[-------------------------------------------------------]
		//[ Synchronization                                       ]
		//[-------------------------------------------------------]
		virtual void flush() override;
		virtual void finish() override;


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		virtual void selfDestruct() override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit NullRenderer(const NullRenderer& source) = delete;
		NullRenderer& operator =(const NullRenderer& source) = delete;

		/**
		*  @brief
		*    Initialize the capabilities
		*/
		void initializeCapabilities();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::IShaderLanguage* mShaderLanguage;			///< Null shader language instance (we keep a reference to it), can be a null pointer
		Renderer::IRenderTarget*   mRenderTarget;			///< Currently set render target (we keep a reference to it), can be a null pointer
		RootSignature*			   mGraphicsRootSignature;	///< Currently set graphics root signature (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/ResourceGroup.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null resource group class
	*/
	class ResourceGroup final : public Renderer::IResourceGroup
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*  @param[in] rootParameterIndex
		*    The root parameter index number for binding
		*  @param[in] numberOfResources
		*    Number of resources, having no resources is invalid
		*  @param[in] resources
		*    At least "numberOfResources" resource pointers, must be valid, the resource group will keep a reference to the resources
		*  @param[in] samplerStates
		*    If not a null pointer at least "numberOfResources" sampler state pointers, must be valid if there's at least one texture resource, the resource group will keep a reference to the sampler states
		*/
		ResourceGroup(Renderer::IRenderer& renderer, uint32_t rootParameterIndex, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates) :
			IResourceGroup(renderer),
			mRootParameterIndex(rootParameterIndex),
			mNumberOfResources(numberOfResources),
			mResources(RENDERER_MALLOC_TYPED(renderer.getContext(), Renderer::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr)
		{
			// Process all resources and add our reference to the renderer resource
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
			{
				Renderer::IResource* resource = *resources;
				mResources[resourceIndex] = resource;
				resource->addReference();
			}
			if (nullptr != samplerStates)
			{
				mSamplerStates = RENDERER_MALLOC_TYPED(renderer.getContext(), Renderer::ISamplerState*, mNumberOfResources);
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Renderer::ISamplerState* samplerState = mSamplerStates[resourceIndex] = samplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->addReference();
					}
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ResourceGroup() override
		{
			// Remove our reference from the renderer resources
			const Renderer::Context& context = getRenderer().getContext();
			if (nullptr != mSamplerStates)
			{
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Renderer::ISamplerState* samplerState = mSamplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->releaseReference();
					}
				}
				RENDERER_FREE(context, mSamplerStates);
			}
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
			{
				mResources[resourceIndex]->releaseReference();
			}
			RENDERER_FREE(context, mResources);
		}

		/**
		*  @brief
		*    Return the number of resources this resource group groups together
		*
		*  @return
		*    The number of resources this resource group groups together
		*/
		inline uint32_t getNumberOfResources() const
		{
			return mNumberOfResources;
		}

		/**
		*  @brief
		*    Return the renderer resources
		*
		*  @return
		*    The renderer resources, don't release or destroy the returned pointer
		*/
		inline Renderer::IResource** getResources() const
		{
			return mResources;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ResourceGroup, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ResourceGroup(const ResourceGroup& source) = delete;
		ResourceGroup& operator =(const ResourceGroup& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t				  mRootParameterIndex;	///< The root parameter index number for binding
		uint32_t				  mNumberOfResources;	///< Number of resources this resource group groups together
		Renderer::IResource**	  mResources;			///< Renderer resources, we keep a reference to it
		Renderer::ISamplerState** mSamplerStates;		///< Sampler states, we keep a reference to it


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/RootSignature.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null root signature ("pipeline layout" in Vulkan terminology) class
	*/
	class RootSignature final : public Renderer::IRootSignature
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(NullRenderer& nullRenderer, const Renderer::RootSignature& rootSignature) :
			IRootSignature(nullRenderer),
			mRootSignature(rootSignature)
		{
			const Renderer::Context& context = nullRenderer.getContext();

			{ // Copy the parameter data
				const uint32_t numberOfParameters = mRootSignature.numberOfParameters;
				if (numberOfParameters > 0)
				{
					mRootSignature.parameters = RENDERER_MALLOC_TYPED(context, Renderer::RootParameter, numberOfParameters);
					Renderer::RootParameter* destinationRootParameters = const_cast<Renderer::RootParameter*>(mRootSignature.parameters);
					memcpy(destinationRootParameters, rootSignature.parameters, sizeof(Renderer::RootParameter) * numberOfParameters);

					// Copy the descriptor table data
					for (uint32_t i = 0; i < numberOfParameters; ++i)
					{
						Renderer::RootParameter& destinationRootParameter = destinationRootParameters[i];
						const Renderer::RootParameter& sourceRootParameter = rootSignature.parameters[i];
						if (Renderer::RootParameterType::DESCRIPTOR_TABLE == destinationRootParameter.parameterType)
						{
							const uint32_t numberOfDescriptorRanges = destinationRootParameter.descriptorTable.numberOfDescriptorRanges;
							destinationRootParameter.descriptorTable.descriptorRanges = reinterpret_cast<uintptr_t>(RENDERER_MALLOC_TYPED(context, Renderer::DescriptorRange, numberOfDescriptorRanges));
							memcpy(reinterpret_cast<Renderer::DescriptorRange*>(destinationRootParameter.descriptorTable.descriptorRanges), reinterpret_cast<const Renderer::DescriptorRange*>(sourceRootParameter.descriptorTable.descriptorRanges), sizeof(Renderer::DescriptorRange) * numberOfDescriptorRanges);
						}
					}
				}
			}

			{ // Copy the static sampler data
				const uint32_t numberOfStaticSamplers = mRootSignature.numberOfStaticSamplers;
				if (numberOfStaticSamplers > 0)
				{
					mRootSignature.staticSamplers = RENDERER_MALLOC_TYPED(context, Renderer::StaticSampler, numberOfStaticSamplers);
					memcpy(const_cast<Renderer::StaticSampler*>(mRootSignature.staticSamplers), rootSignature.staticSamplers, sizeof(Renderer::StaticSampler) * numberOfStaticSamplers);
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~RootSignature() override
		{
			const Renderer::Context& context = getRenderer().getContext();
			if (nullptr != mRootSignature.parameters)
			{
				for (uint32_t i = 0; i < mRootSignature.numberOfParameters; ++i)
				{
					const Renderer::RootParameter& rootParameter = mRootSignature.parameters[i];
					if (Renderer::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RENDERER_FREE(context, reinterpret_cast<Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges));
					}
				}
				RENDERER_FREE(context, const_cast<Renderer::RootParameter*>(mRootSignature.parameters));
			}
			RENDERER_FREE(context, const_cast<Renderer::StaticSampler*>(mRootSignature.staticSamplers));
		}

		/**
		*  @brief
		*    Return the root signature data
		*
		*  @return
		*    The root signature data
		*/
		inline const Renderer::RootSignature& getRootSignature() const
		{
			return mRootSignature;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRootSignature methods       ]
	//[-------------------------------------------------------]
	public:
		virtual Renderer::IResourceGroup* createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, Renderer::IResource** resources, Renderer::ISamplerState** samplerStates = nullptr) override
		{
			// Sanity checks
			RENDERER_ASSERT(getRenderer().getContext(), rootParameterIndex < mRootSignature.numberOfParameters, "The null root parameter index is out-of-bounds")
			RENDERER_ASSERT(getRenderer().getContext(), numberOfResources > 0, "The number of null resources must not be zero")
			RENDERER_ASSERT(getRenderer().getContext(), nullptr != resources, "The null resource pointers must be valid")

			// Create resource group
			return RENDERER_NEW(getRenderer().getContext(), ResourceGroup)(getRenderer(), rootParameterIndex, numberOfResources, resources, samplerStates);
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), RootSignature, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit RootSignature(const RootSignature& source) = delete;
		RootSignature& operator =(const RootSignature& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::RootSignature mRootSignature;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Buffer/IndexBuffer.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null index buffer object (IBO, "element array buffer" in OpenGL terminology) class
	*/
	class IndexBuffer final : public Renderer::IIndexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit IndexBuffer(NullRenderer& nullRenderer) :
			IIndexBuffer(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndexBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), IndexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndexBuffer(const IndexBuffer& source) = delete;
		IndexBuffer& operator =(const IndexBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Buffer/VertexBuffer.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null vertex buffer object (VBO, "array buffer" in OpenGL terminology) class
	*/
	class VertexBuffer final : public Renderer::IVertexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit VertexBuffer(NullRenderer& nullRenderer) :
			IVertexBuffer(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexBuffer(const VertexBuffer& source) = delete;
		VertexBuffer& operator =(const VertexBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Buffer/VertexArray.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null vertex array class
	*/
	class VertexArray final : public Renderer::IVertexArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit VertexArray(NullRenderer& nullRenderer) :
			IVertexArray(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexArray() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexArray, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexArray(const VertexArray& source) = delete;
		VertexArray& operator =(const VertexArray& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Buffer/UniformBuffer.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null uniform buffer object (UBO, "constant buffer" in Direct3D terminology) class
	*/
	class UniformBuffer final : public Renderer::IUniformBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit UniformBuffer(NullRenderer& nullRenderer) :
			IUniformBuffer(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~UniformBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), UniformBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit UniformBuffer(const UniformBuffer& source) = delete;
		UniformBuffer& operator =(const UniformBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Buffer/TextureBuffer.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null texture buffer object (TBO) class
	*/
	class TextureBuffer final : public Renderer::ITextureBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit TextureBuffer(NullRenderer& nullRenderer) :
			ITextureBuffer(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBuffer(const TextureBuffer& source) = delete;
		TextureBuffer& operator =(const TextureBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Buffer/IndirectBuffer.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null indirect buffer object class
	*/
	class IndirectBuffer final : public Renderer::IIndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit IndirectBuffer(NullRenderer& nullRenderer) :
			IIndirectBuffer(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndirectBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IIndirectBuffer methods      ]
	//[-------------------------------------------------------]
	public:
		inline virtual const uint8_t* getEmulationData() const override
		{
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), IndirectBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndirectBuffer(const IndirectBuffer& source) = delete;
		IndirectBuffer& operator =(const IndirectBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Buffer/BufferManager.h                   ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null buffer manager interface
	*/
	class BufferManager final : public Renderer::IBufferManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit BufferManager(NullRenderer& nullRenderer) :
			IBufferManager(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~BufferManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IBufferManager methods       ]
	//[-------------------------------------------------------]
	public:
		inline virtual Renderer::IVertexBuffer* createVertexBuffer(MAYBE_UNUSED uint32_t numberOfBytes, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::DYNAMIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), VertexBuffer)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::IIndexBuffer* createIndexBuffer(MAYBE_UNUSED uint32_t numberOfBytes, MAYBE_UNUSED Renderer::IndexBufferFormat::Enum indexBufferFormat, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::DYNAMIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), IndexBuffer)(static_cast<NullRenderer&>(getRenderer()));
		}

		virtual Renderer::IVertexArray* createVertexArray(MAYBE_UNUSED const Renderer::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Renderer::VertexArrayVertexBuffer* vertexBuffers, Renderer::IIndexBuffer* indexBuffer = nullptr) override
		{
			// We don't keep a reference to the vertex buffers used by the vertex array attributes in here
			// -> Ensure a correct reference counter behaviour
			const Renderer::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
			for (const Renderer::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
			{
				vertexBuffer->vertexBuffer->addReference();
				vertexBuffer->vertexBuffer->releaseReference();
			}

			// We don't keep a reference to the index buffer in here
			// -> Ensure a correct reference counter behaviour
			if (nullptr != indexBuffer)
			{
				indexBuffer->addReference();
				indexBuffer->releaseReference();
			}

			// Create the vertex array instance
			return RENDERER_NEW(getRenderer().getContext(), VertexArray)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::IUniformBuffer* createUniformBuffer(MAYBE_UNUSED uint32_t numberOfBytes, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::DYNAMIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), UniformBuffer)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::ITextureBuffer* createTextureBuffer(MAYBE_UNUSED uint32_t numberOfBytes, MAYBE_UNUSED Renderer::TextureFormat::Enum textureFormat, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::DYNAMIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), TextureBuffer)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::IIndirectBuffer* createIndirectBuffer(MAYBE_UNUSED uint32_t numberOfBytes, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED Renderer::BufferUsage bufferUsage = Renderer::BufferUsage::DYNAMIC_DRAW) override
		{
			return RENDERER_NEW(getRenderer().getContext(), IndirectBuffer)(static_cast<NullRenderer&>(getRenderer()));
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), BufferManager, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit BufferManager(const BufferManager& source) = delete;
		BufferManager& operator =(const BufferManager& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Texture/Texture1D.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null 1D texture class
	*/
	class Texture1D final : public Renderer::ITexture1D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*  @param[in] width
		*    The width of the texture
		*/
		inline Texture1D(NullRenderer& nullRenderer, uint32_t width) :
			ITexture1D(nullRenderer, width)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1D() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture1D, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1D(const Texture1D& source) = delete;
		Texture1D& operator =(const Texture1D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Texture/Texture2D.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null 2D texture class
	*/
	class Texture2D final : public Renderer::ITexture2D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*/
		inline Texture2D(NullRenderer& nullRenderer, uint32_t width, uint32_t height) :
			ITexture2D(nullRenderer, width, height)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2D() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture2D, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2D(const Texture2D& source) = delete;
		Texture2D& operator =(const Texture2D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Texture/Texture2DArray.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null 2D array texture class
	*/
	class Texture2DArray final : public Renderer::ITexture2DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] numberOfSlices
		*    The number of slices
		*/
		inline Texture2DArray(NullRenderer& nullRenderer, uint32_t width, uint32_t height, uint32_t numberOfSlices) :
			ITexture2DArray(nullRenderer, width, height, numberOfSlices)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DArray() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture2DArray, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DArray(const Texture2DArray& source) = delete;
		Texture2DArray& operator =(const Texture2DArray& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Texture/Texture3D.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null 3D texture class
	*/
	class Texture3D final : public Renderer::ITexture3D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] depth
		*    The depth of the texture
		*/
		inline Texture3D(NullRenderer& nullRenderer, uint32_t width, uint32_t height, uint32_t depth) :
			ITexture3D(nullRenderer, width, height, depth)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture3D() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Texture3D, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture3D(const Texture3D& source) = delete;
		Texture3D& operator =(const Texture3D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Texture/TextureCube.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null cube texture class
	*/
	class TextureCube final : public Renderer::ITextureCube
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*/
		inline TextureCube(NullRenderer& nullRenderer, uint32_t width, uint32_t height) :
			ITextureCube(nullRenderer, width, height)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureCube() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureCube, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureCube(const TextureCube& source) = delete;
		TextureCube& operator =(const TextureCube& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Texture/TextureManager.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null texture manager interface
	*/
	class TextureManager final : public Renderer::ITextureManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit TextureManager(NullRenderer& nullRenderer) :
			ITextureManager(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ITextureManager methods      ]
	//[-------------------------------------------------------]
	public:
		virtual Renderer::ITexture1D* createTexture1D(uint32_t width, MAYBE_UNUSED Renderer::TextureFormat::Enum textureFormat, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED uint32_t flags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Check whether or not the given texture dimension is valid
			if (width > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture1D)(static_cast<NullRenderer&>(getRenderer()), width);
			}
			else
			{
				return nullptr;
			}
		}

		virtual Renderer::ITexture2D* createTexture2D(uint32_t width, uint32_t height, MAYBE_UNUSED Renderer::TextureFormat::Enum textureFormat, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED uint32_t flags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT, MAYBE_UNUSED uint8_t numberOfMultisamples = 1, MAYBE_UNUSED const Renderer::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr) override
		{
			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture2D)(static_cast<NullRenderer&>(getRenderer()), width, height);
			}
			else
			{
				return nullptr;
			}
		}

		virtual Renderer::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, MAYBE_UNUSED Renderer::TextureFormat::Enum textureFormat, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED uint32_t flags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0 && numberOfSlices > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture2DArray)(static_cast<NullRenderer&>(getRenderer()), width, height, numberOfSlices);
			}
			else
			{
				return nullptr;
			}
		}

		virtual Renderer::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, MAYBE_UNUSED Renderer::TextureFormat::Enum textureFormat, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED uint32_t flags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0 && depth > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), Texture3D)(static_cast<NullRenderer&>(getRenderer()), width, height, depth);
			}
			else
			{
				return nullptr;
			}
		}

		virtual Renderer::ITextureCube* createTextureCube(uint32_t width, uint32_t height, MAYBE_UNUSED Renderer::TextureFormat::Enum textureFormat, MAYBE_UNUSED const void* data = nullptr, MAYBE_UNUSED uint32_t flags = 0, MAYBE_UNUSED Renderer::TextureUsage textureUsage = Renderer::TextureUsage::DEFAULT) override
		{
			// Check whether or not the given texture dimension is valid
			if (width > 0 && height > 0)
			{
				return RENDERER_NEW(getRenderer().getContext(), TextureCube)(static_cast<NullRenderer&>(getRenderer()), width, height);
			}
			else
			{
				return nullptr;
			}
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TextureManager, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureManager(const TextureManager& source) = delete;
		TextureManager& operator =(const TextureManager& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/State/SamplerState.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null sampler state class
	*/
	class SamplerState final : public Renderer::ISamplerState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit SamplerState(NullRenderer& nullRenderer) :
			ISamplerState(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SamplerState() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), SamplerState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SamplerState(const SamplerState& source) = delete;
		SamplerState& operator =(const SamplerState& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/RenderTarget/RenderPass.h                ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null render pass interface
	*/
	class RenderPass final : public Renderer::IRenderPass
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderer
		*    Owner renderer instance
		*  @param[in] numberOfColorAttachments
		*    Number of color render target textures, must be <="Renderer::Capabilities::maximumNumberOfSimultaneousRenderTargets"
		*  @param[in] colorAttachmentTextureFormats
		*    The color render target texture formats, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "numberOfColorAttachments" textures in the provided C-array of pointers
		*  @param[in] depthStencilAttachmentTextureFormat
		*    The optional depth stencil render target texture format, can be a "Renderer::TextureFormat::UNKNOWN" if there should be no depth buffer
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		RenderPass(Renderer::IRenderer& renderer, uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples) :
			IRenderPass(renderer),
			mNumberOfColorAttachments(numberOfColorAttachments),
			mDepthStencilAttachmentTextureFormat(depthStencilAttachmentTextureFormat),
			mNumberOfMultisamples(numberOfMultisamples)
		{
			RENDERER_ASSERT(renderer.getContext(), mNumberOfColorAttachments < 8, "Invalid number of null color attachments")
			memcpy(mColorAttachmentTextureFormats, colorAttachmentTextureFormats, sizeof(Renderer::TextureFormat::Enum) * mNumberOfColorAttachments);
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~RenderPass() override
		{}

		/**
		*  @brief
		*    Return the number of color render target textures
		*
		*  @return
		*    The number of color render target textures
		*/
		inline uint32_t getNumberOfColorAttachments() const
		{
			return mNumberOfColorAttachments;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), RenderPass, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit RenderPass(const RenderPass& source) = delete;
		RenderPass& operator =(const RenderPass& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		uint32_t					  mNumberOfColorAttachments;
		Renderer::TextureFormat::Enum mColorAttachmentTextureFormats[8];
		Renderer::TextureFormat::Enum mDepthStencilAttachmentTextureFormat;
		uint8_t						  mNumberOfMultisamples;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/RenderTarget/SwapChain.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null swap chain class
	*/
	class SwapChain final : public Renderer::ISwapChain
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*  @param[in] windowHandle
		*    Information about the window to render into
		*/
		inline SwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle) :
			ISwapChain(renderPass),
			mNativeWindowHandle(windowHandle.nativeWindowHandle)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SwapChain() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderTarget methods        ]
	//[-------------------------------------------------------]
	public:
		virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			#ifdef _WIN32
				// Is there a valid native OS window?
				if (NULL_HANDLE != mNativeWindowHandle)
				{
					// Get the width and height
					long swapChainWidth  = 1;
					long swapChainHeight = 1;
					{
						// Get the client rectangle of the native output window
						// -> Don't use the width and height stored in "DXGI_SWAP_CHAIN_DESC" -> "DXGI_MODE_DESC"
						//    because it might have been modified in order to avoid zero values
						RECT rect;
						::GetClientRect(reinterpret_cast<HWND>(mNativeWindowHandle), &rect);

						// Get the width and height...
						swapChainWidth  = rect.right  - rect.left;
						swapChainHeight = rect.bottom - rect.top;

						// ... and ensure that none of them is ever zero
						if (swapChainWidth < 1)
						{
							swapChainWidth = 1;
						}
						if (swapChainHeight < 1)
						{
							swapChainHeight = 1;
						}
					}

					// Write out the width and height
					width  = static_cast<UINT>(swapChainWidth);
					height = static_cast<UINT>(swapChainHeight);
				}
				else
			#elif defined LINUX
				if (mNativeWindowHandle)
				{
					// TODO(sw) Reuse X11 display from "Frontend"
					Display* display = XOpenDisplay(0);

					// Get the width and height...
					::Window rootWindow = 0;
					int positionX = 0, positionY = 0;
					unsigned int unsignedWidth = 0, unsignedHeight = 0, border = 0, depth = 0;
					XGetGeometry(display, mNativeWindowHandle, &rootWindow, &positionX, &positionY, &unsignedWidth, &unsignedHeight, &border, &depth);

					// ... and ensure that none of them is ever zero
					if (unsignedWidth < 1)
					{
						unsignedWidth = 1;
					}
					if (unsignedHeight < 1)
					{
						unsignedHeight = 1;
					}

					// Done
					width = unsignedWidth;
					height = unsignedHeight;
				}
				else
			#else
				#error "Unsupported platform"
			#endif
			{
				// Set known default return values
				width  = 1;
				height = 1;
			}
		}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::ISwapChain methods           ]
	//[-------------------------------------------------------]
	public:
		inline virtual Renderer::handle getNativeWindowHandle() const override
		{
			return mNativeWindowHandle;
		}

		inline virtual void setVerticalSynchronizationInterval(uint32_t) override
		{}

		inline virtual void present() override
		{}

		inline virtual void resizeBuffers() override
		{}

		inline virtual bool getFullscreenState() const override
		{
			// Window mode
			return false;
		}

		inline virtual void setFullscreenState(bool) override
		{}

		inline virtual void setRenderWindow(Renderer::IRenderWindow*) override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), SwapChain, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SwapChain(const SwapChain& source) = delete;
		SwapChain& operator =(const SwapChain& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::handle mNativeWindowHandle;	///< Native window handle window, can be a null handle


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/RenderTarget/Framebuffer.h               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null framebuffer class
	*/
	class Framebuffer final : public Renderer::IFramebuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] renderPass
		*    Render pass to use, the swap chain keeps a reference to the render pass
		*/
		inline explicit Framebuffer(Renderer::IRenderPass& renderPass) :
			IFramebuffer(renderPass)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Framebuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderTarget methods        ]
	//[-------------------------------------------------------]
	public:
		inline virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// TODO(co) Better implementation instead of just returning one (not that important, but would be nice)
			width  = 1;
			height = 1;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Framebuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Framebuffer(const Framebuffer& source) = delete;
		Framebuffer& operator =(const Framebuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Shader/VertexShader.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null vertex shader class
	*/
	class VertexShader final : public Renderer::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit VertexShader(NullRenderer& nullRenderer) :
			IVertexShader(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), VertexShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexShader(const VertexShader& source) = delete;
		VertexShader& operator =(const VertexShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Shader/TessellationControlShader.h       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShader final : public Renderer::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit TessellationControlShader(NullRenderer& nullRenderer) :
			ITessellationControlShader(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TessellationControlShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationControlShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationControlShader(const TessellationControlShader& source) = delete;
		TessellationControlShader& operator =(const TessellationControlShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Shader/TessellationEvaluationShader.h    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShader final : public Renderer::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit TessellationEvaluationShader(NullRenderer& nullRenderer) :
			ITessellationEvaluationShader(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TessellationEvaluationShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), TessellationEvaluationShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationEvaluationShader(const TessellationEvaluationShader& source) = delete;
		TessellationEvaluationShader& operator =(const TessellationEvaluationShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Shader/GeometryShader.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null geometry shader class
	*/
	class GeometryShader final : public Renderer::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit GeometryShader(NullRenderer& nullRenderer) :
			IGeometryShader(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GeometryShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), GeometryShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GeometryShader(const GeometryShader& source) = delete;
		GeometryShader& operator =(const GeometryShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Shader/FragmentShader.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null fragment shader class (FS, "pixel shader" in Direct3D terminology)
	*/
	class FragmentShader final : public Renderer::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit FragmentShader(NullRenderer& nullRenderer) :
			IFragmentShader(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~FragmentShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShader methods              ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), FragmentShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FragmentShader(const FragmentShader& source) = delete;
		FragmentShader& operator =(const FragmentShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Shader/Program.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null program class
	*/
	class Program final : public Renderer::IProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*  @param[in] vertexShader
		*    Vertex shader the program is using, can be a null pointer
		*  @param[in] tessellationControlShader
		*    Tessellation control shader the program is using, can be a null pointer
		*  @param[in] tessellationEvaluationShader
		*    Tessellation evaluation shader the program is using, can be a null pointer
		*  @param[in] geometryShader
		*    Geometry shader the program is using, can be a null pointer
		*  @param[in] fragmentShader
		*    Fragment shader the program is using, can be a null pointer
		*
		*  @note
		*    - The program keeps a reference to the provided shaders and releases it when no longer required
		*/
		Program(NullRenderer& nullRenderer, VertexShader* vertexShader, TessellationControlShader* tessellationControlShader, TessellationEvaluationShader* tessellationEvaluationShader, GeometryShader* geometryShader, FragmentShader* fragmentShader) :
			IProgram(nullRenderer)
		{
			// We don't keep a reference to the shaders in here
			// -> Ensure a correct reference counter behaviour
			if (nullptr != vertexShader)
			{
				vertexShader->addReference();
				vertexShader->releaseReference();
			}
			if (nullptr != tessellationControlShader)
			{
				tessellationControlShader->addReference();
				tessellationControlShader->releaseReference();
			}
			if (nullptr != tessellationEvaluationShader)
			{
				tessellationEvaluationShader->addReference();
				tessellationEvaluationShader->releaseReference();
			}
			if (nullptr != geometryShader)
			{
				geometryShader->addReference();
				geometryShader->releaseReference();
			}
			if (nullptr != fragmentShader)
			{
				fragmentShader->addReference();
				fragmentShader->releaseReference();
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Program() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), Program, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Program(const Program& source) = delete;
		Program& operator =(const Program& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/Shader/ShaderLanguage.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null shader language class
	*/
	class ShaderLanguage final : public Renderer::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*/
		inline explicit ShaderLanguage(NullRenderer& nullRenderer) :
			IShaderLanguage(nullRenderer)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ShaderLanguage() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IShaderLanguage methods      ]
	//[-------------------------------------------------------]
	public:
		inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}

		inline virtual Renderer::IVertexShader* createVertexShaderFromBytecode(MAYBE_UNUSED const Renderer::VertexAttributes& vertexAttributes, MAYBE_UNUSED const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// There's no need to check for "Renderer::Capabilities::vertexShader", we know there's vertex shader support
			return RENDERER_NEW(getRenderer().getContext(), VertexShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::IVertexShader* createVertexShaderFromSourceCode(MAYBE_UNUSED const Renderer::VertexAttributes& vertexAttributes, MAYBE_UNUSED const Renderer::ShaderSourceCode& shaderSourceCode, MAYBE_UNUSED Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::vertexShader", we know there's vertex shader support
			return RENDERER_NEW(getRenderer().getContext(), VertexShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromBytecode(MAYBE_UNUSED const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			return RENDERER_NEW(getRenderer().getContext(), TessellationControlShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::ITessellationControlShader* createTessellationControlShaderFromSourceCode(MAYBE_UNUSED const Renderer::ShaderSourceCode& shaderSourceCode, MAYBE_UNUSED Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			return RENDERER_NEW(getRenderer().getContext(), TessellationControlShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode(MAYBE_UNUSED const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			return RENDERER_NEW(getRenderer().getContext(), TessellationEvaluationShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode(MAYBE_UNUSED const Renderer::ShaderSourceCode& shaderSourceCode, MAYBE_UNUSED Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			return RENDERER_NEW(getRenderer().getContext(), TessellationEvaluationShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::IGeometryShader* createGeometryShaderFromBytecode(MAYBE_UNUSED const Renderer::ShaderBytecode& shaderBytecode, MAYBE_UNUSED Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, MAYBE_UNUSED Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, MAYBE_UNUSED uint32_t numberOfOutputVertices) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			return RENDERER_NEW(getRenderer().getContext(), GeometryShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::IGeometryShader* createGeometryShaderFromSourceCode(MAYBE_UNUSED const Renderer::ShaderSourceCode& shaderSourceCode, MAYBE_UNUSED Renderer::GsInputPrimitiveTopology gsInputPrimitiveTopology, MAYBE_UNUSED Renderer::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, MAYBE_UNUSED uint32_t numberOfOutputVertices, MAYBE_UNUSED Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			return RENDERER_NEW(getRenderer().getContext(), GeometryShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::IFragmentShader* createFragmentShaderFromBytecode(MAYBE_UNUSED const Renderer::ShaderBytecode& shaderBytecode) override
		{
			// There's no need to check for "Renderer::Capabilities::fragmentShader", we know there's fragment shader support
			return RENDERER_NEW(getRenderer().getContext(), FragmentShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		inline virtual Renderer::IFragmentShader* createFragmentShaderFromSourceCode(MAYBE_UNUSED const Renderer::ShaderSourceCode& shaderSourceCode, MAYBE_UNUSED Renderer::ShaderBytecode* shaderBytecode = nullptr) override
		{
			// There's no need to check for "Renderer::Capabilities::fragmentShader", we know there's fragment shader support
			return RENDERER_NEW(getRenderer().getContext(), FragmentShader)(static_cast<NullRenderer&>(getRenderer()));
		}

		virtual Renderer::IProgram* createProgram(MAYBE_UNUSED const Renderer::IRootSignature& rootSignature, MAYBE_UNUSED const Renderer::VertexAttributes& vertexAttributes, Renderer::IVertexShader* vertexShader, Renderer::ITessellationControlShader* tessellationControlShader, Renderer::ITessellationEvaluationShader* tessellationEvaluationShader, Renderer::IGeometryShader* geometryShader, Renderer::IFragmentShader* fragmentShader) override
		{
			// A shader can be a null pointer, but if it's not the shader and program language must match!
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used renderer?
			if (nullptr != vertexShader && vertexShader->getShaderLanguageName() != ::detail::NULL_NAME)
			{
				// Error! Vertex shader language mismatch!
			}
			else if (nullptr != tessellationControlShader && tessellationControlShader->getShaderLanguageName() != ::detail::NULL_NAME)
			{
				// Error! Tessellation control shader language mismatch!
			}
			else if (nullptr != tessellationEvaluationShader && tessellationEvaluationShader->getShaderLanguageName() != ::detail::NULL_NAME)
			{
				// Error! Tessellation evaluation shader language mismatch!
			}
			else if (nullptr != geometryShader && geometryShader->getShaderLanguageName() != ::detail::NULL_NAME)
			{
				// Error! Geometry shader language mismatch!
			}
			else if (nullptr != fragmentShader && fragmentShader->getShaderLanguageName() != ::detail::NULL_NAME)
			{
				// Error! Fragment shader language mismatch!
			}
			else
			{
				return RENDERER_NEW(getRenderer().getContext(), Program)(static_cast<NullRenderer&>(getRenderer()), static_cast<VertexShader*>(vertexShader), static_cast<TessellationControlShader*>(tessellationControlShader), static_cast<TessellationEvaluationShader*>(tessellationEvaluationShader), static_cast<GeometryShader*>(geometryShader), static_cast<FragmentShader*>(fragmentShader));
			}

			// Error! Shader language mismatch!
			// -> Ensure a correct reference counter behaviour, even in the situation of an error
			if (nullptr != vertexShader)
			{
				vertexShader->addReference();
				vertexShader->releaseReference();
			}
			if (nullptr != tessellationControlShader)
			{
				tessellationControlShader->addReference();
				tessellationControlShader->releaseReference();
			}
			if (nullptr != tessellationEvaluationShader)
			{
				tessellationEvaluationShader->addReference();
				tessellationEvaluationShader->releaseReference();
			}
			if (nullptr != geometryShader)
			{
				geometryShader->addReference();
				geometryShader->releaseReference();
			}
			if (nullptr != fragmentShader)
			{
				fragmentShader->addReference();
				fragmentShader->releaseReference();
			}

			// Error!
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), ShaderLanguage, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguage(const ShaderLanguage& source) = delete;
		ShaderLanguage& operator =(const ShaderLanguage& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRenderer/State/PipelineState.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null pipeline state class
	*/
	class PipelineState final : public Renderer::IPipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRenderer
		*    Owner null renderer instance
		*  @param[in] pipelineState
		*    Pipeline state to use
		*/
		PipelineState(NullRenderer& nullRenderer, const Renderer::PipelineState& pipelineState) :
			IPipelineState(nullRenderer),
			mProgram(pipelineState.program),
			mRenderPass(pipelineState.renderPass)
		{
			// Add a reference to the given program and render pass
			mProgram->addReference();
			mRenderPass->addReference();
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~PipelineState() override
		{
			// Release the program reference and render pass
			mProgram->releaseReference();
			mRenderPass->releaseReference();
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RENDERER_DELETE(getRenderer().getContext(), PipelineState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit PipelineState(const PipelineState& source) = delete;
		PipelineState& operator =(const PipelineState& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Renderer::IProgram*	   mProgram;
		Renderer::IRenderPass* mRenderPass;


	};




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // NullRenderer




//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		namespace BackendDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void ExecuteCommandBuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ExecuteCommandBuffer* realData = static_cast<const Renderer::Command::ExecuteCommandBuffer*>(data);
				RENDERER_ASSERT(renderer.getContext(), nullptr != realData->commandBufferToExecute, "The null command buffer to execute must be valid")
				renderer.submitCommandBuffer(*realData->commandBufferToExecute);
			}

			//[-------------------------------------------------------]
			//[ Graphics root                                         ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsRootSignature* realData = static_cast<const Renderer::Command::SetGraphicsRootSignature*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsResourceGroup(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetGraphicsResourceGroup* realData = static_cast<const Renderer::Command::SetGraphicsResourceGroup*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			//[-------------------------------------------------------]
			//[ States                                                ]
			//[-------------------------------------------------------]
			void SetPipelineState(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetPipelineState* realData = static_cast<const Renderer::Command::SetPipelineState*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).setPipelineState(realData->pipelineState);
			}

			//[-------------------------------------------------------]
			//[ Input-assembler (IA) stage                            ]
			//[-------------------------------------------------------]
			void SetVertexArray(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetVertexArray* realData = static_cast<const Renderer::Command::SetVertexArray*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).iaSetVertexArray(realData->vertexArray);
			}

			//[-------------------------------------------------------]
			//[ Rasterizer (RS) stage                                 ]
			//[-------------------------------------------------------]
			void SetViewports(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetViewports* realData = static_cast<const Renderer::Command::SetViewports*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).rsSetViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Renderer::Viewport*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetScissorRectangles(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetScissorRectangles* realData = static_cast<const Renderer::Command::SetScissorRectangles*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).rsSetScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Renderer::ScissorRectangle*>(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			//[-------------------------------------------------------]
			//[ Output-merger (OM) stage                              ]
			//[-------------------------------------------------------]
			void SetRenderTarget(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::SetRenderTarget* realData = static_cast<const Renderer::Command::SetRenderTarget*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).omSetRenderTarget(realData->renderTarget);
			}

			//[-------------------------------------------------------]
			//[ Operations                                            ]
			//[-------------------------------------------------------]
			void Clear(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::Clear* realData = static_cast<const Renderer::Command::Clear*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).clear(realData->flags, realData->color, realData->z, realData->stencil);
			}

			void ResolveMultisampleFramebuffer(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Renderer::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::CopyResource* realData = static_cast<const Renderer::Command::CopyResource*>(data);
				static_cast<NullRenderer::NullRenderer&>(renderer).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			//[-------------------------------------------------------]
			//[ Draw call                                             ]
			//[-------------------------------------------------------]
			void Draw(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::Draw* realData = static_cast<const Renderer::Command::Draw*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<NullRenderer::NullRenderer&>(renderer).drawEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<NullRenderer::NullRenderer&>(renderer).drawEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexed(const void* data, Renderer::IRenderer& renderer)
			{
				const Renderer::Command::Draw* realData = static_cast<const Renderer::Command::Draw*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<NullRenderer::NullRenderer&>(renderer).drawIndexedEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<NullRenderer::NullRenderer&>(renderer).drawIndexedEmulated(Renderer::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void*, Renderer::IRenderer&)
			{}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RENDERER_DEBUG
				void SetDebugMarker(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::SetDebugMarker* realData = static_cast<const Renderer::Command::SetDebugMarker*>(data);
					static_cast<NullRenderer::NullRenderer&>(renderer).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Renderer::IRenderer& renderer)
				{
					const Renderer::Command::BeginDebugEvent* realData = static_cast<const Renderer::Command::BeginDebugEvent*>(data);
					static_cast<NullRenderer::NullRenderer&>(renderer).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Renderer::IRenderer& renderer)
				{
					static_cast<NullRenderer::NullRenderer&>(renderer).endDebugEvent();
				}
			#else
				void SetDebugMarker(const void*, Renderer::IRenderer&)
				{}

				void BeginDebugEvent(const void*, Renderer::IRenderer&)
				{}

				void EndDebugEvent(const void*, Renderer::IRenderer&)
				{}
			#endif


		}


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr Renderer::BackendDispatchFunction DISPATCH_FUNCTIONS[Renderer::CommandDispatchFunctionIndex::NumberOfFunctions] =
		{
			// Command buffer
			&BackendDispatch::ExecuteCommandBuffer,
			// Graphics root
			&BackendDispatch::SetGraphicsRootSignature,
			&BackendDispatch::SetGraphicsResourceGroup,
			// States
			&BackendDispatch::SetPipelineState,
			// Input-assembler (IA) stage
			&BackendDispatch::SetVertexArray,
			// Rasterizer (RS) stage
			&BackendDispatch::SetViewports,
			&BackendDispatch::SetScissorRectangles,
			// Output-merger (OM) stage
			&BackendDispatch::SetRenderTarget,
			// Operations
			&BackendDispatch::Clear,
			&BackendDispatch::ResolveMultisampleFramebuffer,
			&BackendDispatch::CopyResource,
			// Draw call
			&BackendDispatch::Draw,
			&BackendDispatch::DrawIndexed,
			// Resource
			&BackendDispatch::SetTextureMinimumMaximumMipmapIndex,
			// Debug
			&BackendDispatch::SetDebugMarker,
			&BackendDispatch::BeginDebugEvent,
			&BackendDispatch::EndDebugEvent
		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace NullRenderer
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	NullRenderer::NullRenderer(const Renderer::Context& context) :
		IRenderer(Renderer::NameId::NULL_DUMMY, context),
		mShaderLanguage(nullptr),
		mRenderTarget(nullptr),
		mGraphicsRootSignature(nullptr)
	{
		// Initialize the capabilities
		initializeCapabilities();
	}

	NullRenderer::~NullRenderer()
	{
		// Release instances
		if (nullptr != mRenderTarget)
		{
			mRenderTarget->releaseReference();
			mRenderTarget = nullptr;
		}
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
			mGraphicsRootSignature = nullptr;
		}

		#ifndef RENDERER_NO_STATISTICS
		{ // For debugging: At this point there should be no resource instances left, validate this!
			// -> Are the currently any resource instances?
			const unsigned long numberOfCurrentResources = getStatistics().getNumberOfCurrentResources();
			if (numberOfCurrentResources > 0)
			{
				// Error!
				if (numberOfCurrentResources > 1)
				{
					RENDERER_LOG(mContext, CRITICAL, "The null renderer backend is going to be destroyed, but there are still %d resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RENDERER_LOG(mContext, CRITICAL, "The null renderer backend is going to be destroyed, but there is still one resource instance left (memory leak)")
				}

				// Use debug output to show the current number of resource instances
				getStatistics().debugOutputCurrentResouces(mContext);
			}
		}
		#endif

		// Release the null shader language instance, in case we have one
		if (nullptr != mShaderLanguage)
		{
			mShaderLanguage->releaseReference();
		}
	}


	//[-------------------------------------------------------]
	//[ States                                                ]
	//[-------------------------------------------------------]
	void NullRenderer::setGraphicsRootSignature(Renderer::IRootSignature* rootSignature)
	{
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
		}
		mGraphicsRootSignature = static_cast<RootSignature*>(rootSignature);
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->addReference();

			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			NULLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *rootSignature)
		}
	}

	void NullRenderer::setGraphicsResourceGroup(MAYBE_UNUSED uint32_t rootParameterIndex, Renderer::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RENDERER_DEBUG
		{
			if (nullptr == mGraphicsRootSignature)
			{
				RENDERER_LOG(mContext, CRITICAL, "No null renderer backend graphics root signature set")
				return;
			}
			const Renderer::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			if (rootParameterIndex >= rootSignature.numberOfParameters)
			{
				RENDERER_LOG(mContext, CRITICAL, "The null renderer backend root parameter index is out of bounds")
				return;
			}
			const Renderer::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			if (Renderer::RootParameterType::DESCRIPTOR_TABLE != rootParameter.parameterType)
			{
				RENDERER_LOG(mContext, CRITICAL, "The null renderer backend root parameter index doesn't reference a descriptor table")
				return;
			}
			if (nullptr == reinterpret_cast<const Renderer::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges))
			{
				RENDERER_LOG(mContext, CRITICAL, "The null renderer backend descriptor ranges is a null pointer")
				return;
			}
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			NULLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *resourceGroup)

			// TODO(co) Some additional resource type root signature security checks in debug build?
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void NullRenderer::setPipelineState(Renderer::IPipelineState* pipelineState)
	{
		if (nullptr != pipelineState)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			NULLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *pipelineState)
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}


	//[-------------------------------------------------------]
	//[ Input-assembler (IA) stage                            ]
	//[-------------------------------------------------------]
	void NullRenderer::iaSetVertexArray(Renderer::IVertexArray* vertexArray)
	{
		// Nothing here, the following is just for debugging
		if (nullptr != vertexArray)
		{
			// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
			NULLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *vertexArray)
		}
	}


	//[-------------------------------------------------------]
	//[ Rasterizer (RS) stage                                 ]
	//[-------------------------------------------------------]
	void NullRenderer::rsSetViewports(MAYBE_UNUSED uint32_t numberOfViewports, MAYBE_UNUSED const Renderer::Viewport* viewports)
	{
		// Sanity check
		RENDERER_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid null rasterizer state viewports")
	}

	void NullRenderer::rsSetScissorRectangles(MAYBE_UNUSED uint32_t numberOfScissorRectangles, MAYBE_UNUSED const Renderer::ScissorRectangle* scissorRectangles)
	{
		// Sanity check
		RENDERER_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid null rasterizer state scissor rectangles")
	}


	//[-------------------------------------------------------]
	//[ Output-merger (OM) stage                              ]
	//[-------------------------------------------------------]
	void NullRenderer::omSetRenderTarget(Renderer::IRenderTarget* renderTarget)
	{
		// New render target?
		if (mRenderTarget != renderTarget)
		{
			// Set a render target?
			if (nullptr != renderTarget)
			{
				// Security check: Is the given resource owned by this renderer? (calls "return" in case of a mismatch)
				NULLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, *renderTarget)

				// Release the render target reference, in case we have one
				if (nullptr != mRenderTarget)
				{
					mRenderTarget->releaseReference();
				}

				// Set new render target and add a reference to it
				mRenderTarget = renderTarget;
				mRenderTarget->addReference();

				// That's all folks!
			}
			else
			{
				// Release the render target reference, in case we have one
				if (nullptr != mRenderTarget)
				{
					mRenderTarget->releaseReference();
					mRenderTarget = nullptr;
				}

				// That's all folks!
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	void NullRenderer::clear(uint32_t, const float [4], float, uint32_t)
	{}

	void NullRenderer::resolveMultisampleFramebuffer(Renderer::IRenderTarget&, Renderer::IFramebuffer&)
	{}

	void NullRenderer::copyResource(Renderer::IResource&, Renderer::IResource&)
	{
		// TODO(co) Implement me
	}


	//[-------------------------------------------------------]
	//[ Draw call                                             ]
	//[-------------------------------------------------------]
	void NullRenderer::drawEmulated(MAYBE_UNUSED const uint8_t* emulationData, uint32_t, MAYBE_UNUSED uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The null emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of null draws must not be zero")
	}

	void NullRenderer::drawIndexedEmulated(MAYBE_UNUSED const uint8_t* emulationData, uint32_t, MAYBE_UNUSED uint32_t numberOfDraws)
	{
		// Sanity checks
		RENDERER_ASSERT(mContext, nullptr != emulationData, "The null emulation data must be valid")
		RENDERER_ASSERT(mContext, numberOfDraws > 0, "The number of null draws must not be zero")
	}


	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RENDERER_DEBUG
		void NullRenderer::setDebugMarker(const char*)
		{}

		void NullRenderer::beginDebugEvent(const char*)
		{}

		void NullRenderer::endDebugEvent()
		{}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Renderer::IRenderer methods            ]
	//[-------------------------------------------------------]
	const char* NullRenderer::getName() const
	{
		return "Null";
	}

	bool NullRenderer::isInitialized() const
	{
		return true;
	}

	bool NullRenderer::isDebugEnabled()
	{
		// Nothing here

		// Debug disabled
		return false;
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t NullRenderer::getNumberOfShaderLanguages() const
	{
		// Only one shader language supported in here
		return 1;
	}

	const char* NullRenderer::getShaderLanguageName(uint32_t index) const
	{
		// Only one shader language supported in here
		if (0 == index)
		{
			return ::detail::NULL_NAME;
		}
		else
		{
			// Error!
			return nullptr;
		}
	}

	Renderer::IShaderLanguage* NullRenderer::getShaderLanguage(const char* shaderLanguageName)
	{
		// In case "shaderLanguage" is a null pointer, use the default shader language
		if (nullptr != shaderLanguageName)
		{
			// In case "shaderLanguage" is a null pointer, use the default shader language
			// -> Only one shader language supported in here
			if (nullptr == shaderLanguageName || !stricmp(shaderLanguageName, ::detail::NULL_NAME))
			{
				// If required, create the null shader language instance right now
				if (nullptr == mShaderLanguage)
				{
					mShaderLanguage = RENDERER_NEW(mContext, ShaderLanguage)(*this);
					mShaderLanguage->addReference();	// Internal renderer reference
				}

				// Return the shader language instance
				return mShaderLanguage;
			}

			// Error!
			return nullptr;
		}

		// Return the null shader language instance as default
		return getShaderLanguage(::detail::NULL_NAME);
	}


	//[-------------------------------------------------------]
	//[ Resource creation                                     ]
	//[-------------------------------------------------------]
	Renderer::IRenderPass* NullRenderer::createRenderPass(uint32_t numberOfColorAttachments, const Renderer::TextureFormat::Enum* colorAttachmentTextureFormats, Renderer::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples)
	{
		return RENDERER_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples);
	}

	Renderer::ISwapChain* NullRenderer::createSwapChain(Renderer::IRenderPass& renderPass, Renderer::WindowHandle windowHandle, bool)
	{
		// Sanity checks
		NULLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)
		RENDERER_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle, "Null: The provided native window handle must not be a null handle")

		// Create the swap chain
		return RENDERER_NEW(mContext, SwapChain)(renderPass, windowHandle);
	}

	Renderer::IFramebuffer* NullRenderer::createFramebuffer(Renderer::IRenderPass& renderPass, const Renderer::FramebufferAttachment* colorFramebufferAttachments, const Renderer::FramebufferAttachment* depthStencilFramebufferAttachment)
	{
		// Sanity check
		NULLRENDERER_RENDERERMATCHCHECK_ASSERT(*this, renderPass)

		// We don't keep a reference to the provided textures in here
		// -> Ensure a correct reference counter behaviour

		// Are there any color textures?
		const uint32_t numberOfColorAttachments = static_cast<RenderPass&>(renderPass).getNumberOfColorAttachments();
		if (numberOfColorAttachments > 0)
		{
			// Loop through all color textures
			const Renderer::FramebufferAttachment* colorFramebufferAttachmentsEnd = colorFramebufferAttachments + numberOfColorAttachments;
			for (const Renderer::FramebufferAttachment* colorFramebufferAttachment = colorFramebufferAttachments; colorFramebufferAttachment < colorFramebufferAttachmentsEnd; ++colorFramebufferAttachment)
			{
				// Valid entry?
				if (nullptr != colorFramebufferAttachment->texture)
				{
					// TODO(co) Add security check: Is the given resource one of the currently used renderer?
					colorFramebufferAttachment->texture->addReference();
					colorFramebufferAttachment->texture->releaseReference();
				}
			}
		}

		// Add a reference to the used depth stencil texture
		if (nullptr != depthStencilFramebufferAttachment)
		{
			depthStencilFramebufferAttachment->texture->addReference();
			depthStencilFramebufferAttachment->texture->releaseReference();
		}

		// Create the framebuffer instance
		return RENDERER_NEW(mContext, Framebuffer)(renderPass);
	}

	Renderer::IBufferManager* NullRenderer::createBufferManager()
	{
		return RENDERER_NEW(mContext, BufferManager)(*this);
	}

	Renderer::ITextureManager* NullRenderer::createTextureManager()
	{
		return RENDERER_NEW(mContext, TextureManager)(*this);
	}

	Renderer::IRootSignature* NullRenderer::createRootSignature(const Renderer::RootSignature& rootSignature)
	{
		return RENDERER_NEW(mContext, RootSignature)(*this, rootSignature);
	}

	Renderer::IPipelineState* NullRenderer::createPipelineState(const Renderer::PipelineState& pipelineState)
	{
		return RENDERER_NEW(mContext, PipelineState)(*this, pipelineState);
	}

	Renderer::ISamplerState* NullRenderer::createSamplerState(const Renderer::SamplerState &)
	{
		return RENDERER_NEW(mContext, SamplerState)(*this);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool NullRenderer::map(Renderer::IResource&, uint32_t, Renderer::MapType, uint32_t, Renderer::MappedSubresource&)
	{
		// Not supported by the null renderer
		return false;
	}

	void NullRenderer::unmap(Renderer::IResource&, uint32_t)
	{}


	//[-------------------------------------------------------]
	//[ Operations                                            ]
	//[-------------------------------------------------------]
	bool NullRenderer::beginScene()
	{
		// Nothing here

		// Done
		return true;
	}

	void NullRenderer::submitCommandBuffer(const Renderer::CommandBuffer& commandBuffer)
	{
		// Loop through all commands
		const uint8_t* commandPacketBuffer = commandBuffer.getCommandPacketBuffer();
		Renderer::ConstCommandPacket constCommandPacket = commandPacketBuffer;
		while (nullptr != constCommandPacket)
		{
			{ // Submit command packet
				const Renderer::CommandDispatchFunctionIndex commandDispatchFunctionIndex = Renderer::CommandPacketHelper::loadCommandDispatchFunctionIndex(constCommandPacket);
				const void* command = Renderer::CommandPacketHelper::loadCommand(constCommandPacket);
				detail::DISPATCH_FUNCTIONS[commandDispatchFunctionIndex](command, *this);
			}

			{ // Next command
				const uint32_t nextCommandPacketByteIndex = Renderer::CommandPacketHelper::getNextCommandPacketByteIndex(constCommandPacket);
				constCommandPacket = (~0u != nextCommandPacketByteIndex) ? &commandPacketBuffer[nextCommandPacketByteIndex] : nullptr;
			}
		}
	}

	void NullRenderer::endScene()
	{
		// We need to forget about the currently set render target
		omSetRenderTarget(nullptr);
	}


	//[-------------------------------------------------------]
	//[ Synchronization                                       ]
	//[-------------------------------------------------------]
	void NullRenderer::flush()
	{}

	void NullRenderer::finish()
	{}


	//[-------------------------------------------------------]
	//[ Protected virtual Renderer::RefCount methods          ]
	//[-------------------------------------------------------]
	void NullRenderer::selfDestruct()
	{
		RENDERER_DELETE(mContext, NullRenderer, this);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void NullRenderer::initializeCapabilities()
	{
		strcpy(mCapabilities.deviceName, "Null");

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat		  = Renderer::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Renderer::TextureFormat::Enum::D32_FLOAT;

		// Maximum number of viewports (always at least 1)
		mCapabilities.maximumNumberOfViewports = 1;

		// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
		mCapabilities.maximumNumberOfSimultaneousRenderTargets = 8;

		// Maximum texture dimension
		mCapabilities.maximumTextureDimension = 42;

		// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
		mCapabilities.maximumNumberOf2DTextureArraySlices = 42;

		// Maximum uniform buffer (UBO) size in bytes (usually at least 4096 * 16 bytes, in case there's no support for uniform buffer it's 0)
		// -> Let's use the DirectX 11 value: See https://msdn.microsoft.com/en-us/library/windows/desktop/ff819065(v=vs.85).aspx - "Resource Limits (Direct3D 11)" - "Number of elements in a constant buffer D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT (4096)"
		// -> One element = float4 = 16 bytes
		mCapabilities.maximumUniformBufferSize = 4096 * 16;

		// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
		mCapabilities.maximumTextureBufferSize = 42;

		// Maximum indirect buffer size in bytes
		mCapabilities.maximumIndirectBufferSize = 64 * 1024;	// 64 KiB

		// Maximum number of multisamples (always at least 1, usually 8)
		mCapabilities.maximumNumberOfMultisamples = 1;

		// Maximum anisotropy (always at least 1, usually 16)
		mCapabilities.maximumAnisotropy = 16;

		// Left-handed coordinate system with clip space depth value range 0..1
		mCapabilities.upperLeftOrigin = mCapabilities.zeroToOneClipZ = true;

		// Individual uniforms ("constants" in Direct3D terminology) supported? If not, only uniform buffer objects are supported.
		mCapabilities.individualUniforms = true;

		// Instanced arrays supported? (shader model 3 feature, vertex array element advancing per-instance instead of per-vertex)
		mCapabilities.instancedArrays = true;

		// Draw instanced supported? (shader model 4 feature, build in shader variable holding the current instance ID)
		mCapabilities.drawInstanced = true;

		// Base vertex supported for draw calls?
		mCapabilities.baseVertex = true;

		// The null renderer has native multi-threading
		mCapabilities.nativeMultiThreading = true;

		// The null renderer has no shader bytecode support
		mCapabilities.shaderBytecode = false;

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
		mCapabilities.maximumNumberOfPatchVertices = 32;

		// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
		mCapabilities.maximumNumberOfGsOutputVertices = 1024;

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // NullRenderer




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef NULLRENDERER_EXPORTS
	#define NULLRENDERER_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define NULLRENDERER_FUNCTION_EXPORT
#endif
NULLRENDERER_FUNCTION_EXPORT Renderer::IRenderer* createNullRendererInstance(const Renderer::Context& context)
{
	return RENDERER_NEW(context, NullRenderer::NullRenderer)(context);
}
#undef NULLRENDERER_FUNCTION_EXPORT
