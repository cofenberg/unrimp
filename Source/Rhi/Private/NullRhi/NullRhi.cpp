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


/**
*  @brief
*    Null RHI amalgamated/unity build implementation
*
*  @remarks
*    == Dependencies ==
*    None.
*
*    == Preprocessor Definitions ==
*    - Set "RHI_NULL_EXPORTS" as preprocessor definition when building this library as shared library
*    - Do also have a look into the RHI header file documentation
*/


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <Rhi/Public/Rhi.h>

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
	#include <Windows.h>

	// Get rid of some nasty OS macros
	#undef max
#elif LINUX
	// TODO(co) Review which of the following headers can be removed
	#include <X11/Xlib.h>
#endif




//[-------------------------------------------------------]
//[ NullRhi/MakeID.h                                      ]
//[-------------------------------------------------------]
/*

Author:
	Emil Persson, A.K.A. Humus.
	http://www.humus.name

Version history:
	1.0  - Initial release.
	1.01 - Code review fixes. Code reviewed by Denis A. Gladkiy.
	1.02 - Fixed an off-by-one error in DestroyRange() found by Markus Billeter

License:
	Public Domain

	This file is released in the hopes that it will be useful. Use in whatever way you like, but no guarantees that it
	actually works or fits any particular purpose. It has been unit-tested and benchmarked though, and seems to do
	what it was designed to do, and seems pretty quick at it too.

Notes: 
	There are many applications where it is desired to generate unique IDs at runtime for various resources, such that they can be
	distinguished, sorted or otherwise processed in an efficient manner. It can in some cases replace hashes, handles and pointers.
	In cases where resource pointers are used as IDs, it offers a unique ID that requires far fewer bits, especially for 64bit apps.
	The design goal of this implementation was to return the most compact IDs as possible, limiting to a specific range if necessary.

	The properties of this system are as follows:
		- Creating a new ID returns the smallest possible unused ID.
		- Creating a new range of IDs returns the smallest possible continuous range of the specified size.
		- Created IDs remain valid until destroyed.
		- Destroying an ID returns it to the pool and may be returned by subsequent allocations.
		- The system is NOT thread-safe.

	Performance properties:
		- Creating an ID is O(1) and generally super-cheap.
		- Destroying an ID is also cheap, but O(log(n)), where n is the current number of distinct available ranges.
		- The system merges available ranges when IDs are destroyed, keeping said n generally very small in practice.
		- After warmup, no further memory allocations should be necessary, or be very rare.
		- The system uses very little memory.
		- It is possible to construct a pathological case where fragmentation would cause n to become large. This can be done by
		  first allocating a very large range of IDs, then deleting every other ID, causing a new range to be allocated for every
		  free ID, or as many ranges as there are free IDs. I believe nothing close to this situation happens in practical applications.
		  In tests, millions of random scattered creations and deletions only resulted in a relatively short list in the worst case.
		  This is because freed IDs are quickly reused and ranges eagerly merged.

	Where would this system be useful? It was originally thought up as a replacement for resource pointers as part of sort-ids
	in rendering. Using for instance a 64-bit sort-id packing various flags and states, putting a pointer in there takes an
	awful lot of bits, especially considering the actual possible resources range in the thousands at most. This got far worse
	of course with the switch to 64bit as pointers are now twice as large and essentially eats all bits except bottom few for
	alignment.
	Another application would be for managing a shared pool of resources. IDs could be handed out as handles and used to access
	the actual resource from an array. By always returning the lowest possible ID or range of IDs we get very good cache behavior
	since all active resources will grouped together in the bottom part of the array. Using IDs instead of pointers for handles
	also allows easy resizing of the allocated memory since IDs can remain the same even if the underlying storage changed.

*/


#ifdef RHI_DEBUG
	#include <cstdio>  // For printf(). Remove if you don't need the PrintRanges() function (mostly for debugging anyway).
#endif

#include <cstdint>	// uint32_t
#include <limits>	// std::numeric_limits<type>::max()
#include <cstdlib>
#include <cstring>

class MakeID final
{
private:
	// Change to uint16_t here for a more compact implementation if 16bit or less IDs work for you.
	typedef uint16_t uint;

	struct Range
	{
		uint m_First;
		uint m_Last;
	};

	Rhi::IAllocator& m_Allocator;
	Range *m_Ranges; // Sorted array of ranges of free IDs
	uint m_Count;    // Number of ranges in list
	uint m_Capacity; // Total capacity of range list

	MakeID & operator=(const MakeID &) = delete;
	MakeID(const MakeID &) = delete;

public:
	MakeID(Rhi::IAllocator& allocator, const uint max_id = std::numeric_limits<uint>::max()) :
		m_Allocator(allocator),
		m_Ranges(static_cast<Range*>(allocator.reallocate(nullptr, 0, sizeof(Range), 1))),
		m_Count(1),
		m_Capacity(1)
	{
		// Start with a single range, from 0 to max allowed ID (specified)
		m_Ranges[0].m_First = 0;
		m_Ranges[0].m_Last = max_id;
	}

	~MakeID()
	{
		m_Allocator.reallocate(m_Ranges, 0, 0, 1);
	}

	bool CreateID(uint &id)
	{
		if (m_Ranges[0].m_First <= m_Ranges[0].m_Last)
		{
			id = m_Ranges[0].m_First;

			// If current range is full and there is another one, that will become the new current range
			if (m_Ranges[0].m_First == m_Ranges[0].m_Last && m_Count > 1)
			{
				DestroyRange(0);
			}
			else
			{
				++m_Ranges[0].m_First;
			}
			return true;
		}

		// No available ID left
		return false;
	}

	bool CreateRangeID(uint &id, const uint count)
	{
		uint i = 0;
		do
		{
			const uint range_count = 1u + m_Ranges[i].m_Last - m_Ranges[i].m_First;
			if (count <= range_count)
			{
				id = m_Ranges[i].m_First;

				// If current range is full and there is another one, that will become the new current range
				if (count == range_count && i + 1 < m_Count)
				{
					DestroyRange(i);
				}
				else
				{
					m_Ranges[i].m_First += count;
				}
				return true;
			}
			++i;
		} while (i < m_Count);

		// No range of free IDs was large enough to create the requested continuous ID sequence
		return false;
	}

	bool DestroyID(const uint id)
	{
		return DestroyRangeID(id, 1);
	}

	bool DestroyRangeID(const uint id, const uint count)
	{
		const uint end_id = static_cast<uint>(id + count);

		// Binary search of the range list
		uint i0 = 0u;
		uint i1 = m_Count - 1u;

		for (;;)
		{
			const uint i = (i0 + i1) / 2u;

			if (id < m_Ranges[i].m_First)
			{
				// Before current range, check if neighboring
				if (end_id >= m_Ranges[i].m_First)
				{
					if (end_id != m_Ranges[i].m_First)
						return false; // Overlaps a range of free IDs, thus (at least partially) invalid IDs

					// Neighbor id, check if neighboring previous range too
					if (i > i0 && id - 1 == m_Ranges[i - 1].m_Last)
					{
						// Merge with previous range
						m_Ranges[i - 1].m_Last = m_Ranges[i].m_Last;
						DestroyRange(i);
					}
					else
					{
						// Just grow range
						m_Ranges[i].m_First = id;
					}
					return true;
				}
				else
				{
					// Non-neighbor id
					if (i != i0)
					{
						// Cull upper half of list
						i1 = i - 1u;
					}
					else
					{
						// Found our position in the list, insert the deleted range here
						InsertRange(i);
						m_Ranges[i].m_First = id;
						m_Ranges[i].m_Last = end_id - 1u;
						return true;
					}
				}
			}
			else if (id > m_Ranges[i].m_Last)
			{
				// After current range, check if neighboring
				if (id - 1 == m_Ranges[i].m_Last)
				{
					// Neighbor id, check if neighboring next range too
					if (i < i1 && end_id == m_Ranges[i + 1].m_First)
					{
						// Merge with next range
						m_Ranges[i].m_Last = m_Ranges[i + 1].m_Last;
						DestroyRange(i + 1u);
					}
					else
					{
						// Just grow range
						m_Ranges[i].m_Last += count;
					}
					return true;
				}
				else
				{
					// Non-neighbor id
					if (i != i1)
					{
						// Cull bottom half of list
						i0 = i + 1u;
					}
					else
					{
						// Found our position in the list, insert the deleted range here
						InsertRange(i + 1u);
						m_Ranges[i + 1].m_First = id;
						m_Ranges[i + 1].m_Last = end_id - 1u;
						return true;
					}
				}
			}
			else
			{
				// Inside a free block, not a valid ID
				return false;
			}

		}
	}

	bool IsID(const uint id) const
	{
		// Binary search of the range list
		uint i0 = 0u;
		uint i1 = m_Count - 1u;

		for (;;)
		{
			const uint i = (i0 + i1) / 2u;

			if (id < m_Ranges[i].m_First)
			{
				if (i == i0)
					return true;

				// Cull upper half of list
				i1 = i - 1u;
			}
			else if (id > m_Ranges[i].m_Last)
			{
				if (i == i1)
					return true;

				// Cull bottom half of list
				i0 = i + 1u;
			}
			else
			{
				// Inside a free block, not a valid ID
				return false;
			}

		}
	}

	uint GetAvailableIDs() const
	{
		uint count = m_Count;
		uint i = 0;

		do
		{
			count += m_Ranges[i].m_Last - m_Ranges[i].m_First;
			++i;
		} while (i < m_Count);

		return count;
	}

	uint GetLargestContinuousRange() const
	{
		uint max_count = 0;
		uint i = 0;

		do
		{
			uint count = m_Ranges[i].m_Last - m_Ranges[i].m_First + 1u;
			if (count > max_count)
				max_count = count;

			++i;
		} while (i < m_Count);

		return max_count;
	}

	#ifdef RHI_DEBUG
		void PrintRanges() const
		{
			uint i = 0;
			for (;;)
			{
				if (m_Ranges[i].m_First < m_Ranges[i].m_Last)
					printf("%u-%u", m_Ranges[i].m_First, m_Ranges[i].m_Last);
				else if (m_Ranges[i].m_First == m_Ranges[i].m_Last)
					printf("%u", m_Ranges[i].m_First);
				else
					printf("-");

				++i;
				if (i >= m_Count)
				{
					printf("\n");
					return;
				}

				printf(", ");
			}
		}
	#endif


private:

	void InsertRange(const uint index)
	{
		if (m_Count >= m_Capacity)
		{
			m_Ranges = static_cast<Range *>(m_Allocator.reallocate(m_Ranges, sizeof(Range) * m_Capacity, (m_Capacity + m_Capacity) * sizeof(Range), 1));
			m_Capacity += m_Capacity;
		}
 
		::memmove(m_Ranges + index + 1, m_Ranges + index, (m_Count - index) * sizeof(Range));
		++m_Count;
	}

	void DestroyRange(const uint index)
	{
		--m_Count;
		::memmove(m_Ranges + index, m_Ranges + index + 1, (m_Count - index) * sizeof(Range));
	}
};




//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace NullRhi
{
	class RootSignature;
}




//[-------------------------------------------------------]
//[ Macros & definitions                                  ]
//[-------------------------------------------------------]
#ifdef RHI_DEBUG
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given RHI
	*/
	#define RHI_MATCH_CHECK(rhiReference, resourceReference) \
		RHI_ASSERT(mContext, &rhiReference == &(resourceReference).getRhi(), "Null error: The given resource is owned by another RHI instance")

	/**
	*  @brief
	*    Resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*
	*  @param[in] debugName
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT , [[maybe_unused]] const char debugName[]
#else
	/*
	*  @brief
	*    Check whether or not the given resource is owned by the given RHI
	*/
	#define RHI_MATCH_CHECK(rhiReference, resourceReference)

	/**
	*  @brief
	*    Resource name for debugging purposes, ignored when not using "RHI_DEBUG"
	*
	*  @param[in] debugName
	*    ASCII name for debugging purposes, must be valid (there's no internal null pointer test)
	*/
	#define RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT
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
namespace NullRhi
{




	//[-------------------------------------------------------]
	//[ NullRhi/NullRhi.h                                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null RHI class
	*/
	class NullRhi final : public Rhi::IRhi
	{


	//[-------------------------------------------------------]
	//[ Public data                                           ]
	//[-------------------------------------------------------]
	public:
		MakeID VertexArrayMakeId;
		MakeID GraphicsPipelineStateMakeId;
		MakeID ComputePipelineStateMakeId;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] context
		*    RHI context, the RHI context instance must stay valid as long as the RHI instance exists
		*
		*  @note
		*    - Do never ever use a not properly initialized RHI. Use "Rhi::IRhi::isInitialized()" to check the initialization state.
		*/
		explicit NullRhi(const Rhi::Context& context);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~NullRhi() override;

		void dispatchCommandBufferInternal(const Rhi::CommandBuffer& commandBuffer);

		//[-------------------------------------------------------]
		//[ Graphics                                              ]
		//[-------------------------------------------------------]
		void setGraphicsRootSignature(Rhi::IRootSignature* rootSignature);
		void setGraphicsPipelineState(Rhi::IGraphicsPipelineState* graphicsPipelineState);
		void setGraphicsResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup);
		void setGraphicsVertexArray(Rhi::IVertexArray* vertexArray);															// Input-assembler (IA) stage
		void setGraphicsViewports(uint32_t numberOfViewports, const Rhi::Viewport* viewports);									// Rasterizer (RS) stage
		void setGraphicsScissorRectangles(uint32_t numberOfScissorRectangles, const Rhi::ScissorRectangle* scissorRectangles);	// Rasterizer (RS) stage
		void setGraphicsRenderTarget(Rhi::IRenderTarget* renderTarget);															// Output-merger (OM) stage
		void clearGraphics(uint32_t clearFlags, const float color[4], float z, uint32_t stencil);
		void drawGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawIndexedGraphicsEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		void drawMeshTasksEmulated(const uint8_t* emulationData, uint32_t indirectBufferOffset = 0, uint32_t numberOfDraws = 1);
		//[-------------------------------------------------------]
		//[ Compute                                               ]
		//[-------------------------------------------------------]
		void setComputeRootSignature(Rhi::IRootSignature* rootSignature);
		void setComputePipelineState(Rhi::IComputePipelineState* computePipelineState);
		void setComputeResourceGroup(uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup);
		void dispatchCompute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		//[-------------------------------------------------------]
		//[ Resource                                              ]
		//[-------------------------------------------------------]
		void resolveMultisampleFramebuffer(Rhi::IRenderTarget& destinationRenderTarget, Rhi::IFramebuffer& sourceMultisampleFramebuffer);
		void copyResource(Rhi::IResource& destinationResource, Rhi::IResource& sourceResource);
		void generateMipmaps(Rhi::IResource& resource);
		//[-------------------------------------------------------]
		//[ Query                                                 ]
		//[-------------------------------------------------------]
		void resetQueryPool(Rhi::IQueryPool& queryPool, uint32_t firstQueryIndex, uint32_t numberOfQueries);
		void beginQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex, uint32_t queryControlFlags);
		void endQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex);
		void writeTimestampQuery(Rhi::IQueryPool& queryPool, uint32_t queryIndex);
		//[-------------------------------------------------------]
		//[ Debug                                                 ]
		//[-------------------------------------------------------]
		#ifdef RHI_DEBUG
			void setDebugMarker(const char* name);
			void beginDebugEvent(const char* name);
			void endDebugEvent();
		#endif


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRhi methods                      ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual const char* getName() const override;
		[[nodiscard]] virtual bool isInitialized() const override;
		[[nodiscard]] virtual bool isDebugEnabled() override;
		//[-------------------------------------------------------]
		//[ Shader language                                       ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual uint32_t getNumberOfShaderLanguages() const override;
		[[nodiscard]] virtual const char* getShaderLanguageName(uint32_t index) const override;
		[[nodiscard]] virtual Rhi::IShaderLanguage* getShaderLanguage(const char* shaderLanguageName = nullptr) override;
		//[-------------------------------------------------------]
		//[ Resource creation                                     ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual Rhi::IRenderPass* createRenderPass(uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat = Rhi::TextureFormat::UNKNOWN, uint8_t numberOfMultisamples = 1 RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IQueryPool* createQueryPool(Rhi::QueryType queryType, uint32_t numberOfQueries = 1 RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::ISwapChain* createSwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle, bool useExternalContext = false RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IFramebuffer* createFramebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IBufferManager* createBufferManager() override;
		[[nodiscard]] virtual Rhi::ITextureManager* createTextureManager() override;
		[[nodiscard]] virtual Rhi::IRootSignature* createRootSignature(const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IGraphicsPipelineState* createGraphicsPipelineState(const Rhi::GraphicsPipelineState& graphicsPipelineState RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::IComputePipelineState* createComputePipelineState(Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		[[nodiscard]] virtual Rhi::ISamplerState* createSamplerState(const Rhi::SamplerState& samplerState RHI_RESOURCE_DEBUG_NAME_PARAMETER) override;
		//[-------------------------------------------------------]
		//[ Resource handling                                     ]
		//[-------------------------------------------------------]
		[[nodiscard]] virtual bool map(Rhi::IResource& resource, uint32_t subresource, Rhi::MapType mapType, uint32_t mapFlags, Rhi::MappedSubresource& mappedSubresource) override;
		virtual void unmap(Rhi::IResource& resource, uint32_t subresource) override;
		[[nodiscard]] virtual bool getQueryPoolResults(Rhi::IQueryPool& queryPool, uint32_t numberOfDataBytes, uint8_t* data, uint32_t firstQueryIndex = 0, uint32_t numberOfQueries = 1, uint32_t strideInBytes = 0, uint32_t queryResultFlags = 0) override;
		//[-------------------------------------------------------]
		//[ Operation                                             ]
		//[-------------------------------------------------------]
		virtual void dispatchCommandBuffer(const Rhi::CommandBuffer& commandBuffer) override;


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		virtual void selfDestruct() override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit NullRhi(const NullRhi& source) = delete;
		NullRhi& operator =(const NullRhi& source) = delete;

		/**
		*  @brief
		*    Initialize the capabilities
		*/
		void initializeCapabilities();


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::IShaderLanguage* mShaderLanguage;			///< Null shader language instance (we keep a reference to it), can be a null pointer
		Rhi::IRenderTarget*   mRenderTarget;			///< Currently set render target (we keep a reference to it), can be a null pointer
		RootSignature*		  mGraphicsRootSignature;	///< Currently set graphics root signature (we keep a reference to it), can be a null pointer
		RootSignature*		  mComputeRootSignature;	///< Currently set compute root signature (we keep a reference to it), can be a null pointer


	};




	//[-------------------------------------------------------]
	//[ NullRhi/ResourceGroup.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null resource group class
	*/
	class ResourceGroup final : public Rhi::IResourceGroup
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] rhi
		*    Owner RHI instance
		*  @param[in] rootParameterIndex
		*    The root parameter index number for binding
		*  @param[in] numberOfResources
		*    Number of resources, having no resources is invalid
		*  @param[in] resources
		*    At least "numberOfResources" resource pointers, must be valid, the resource group will keep a reference to the resources
		*  @param[in] samplerStates
		*    If not a null pointer at least "numberOfResources" sampler state pointers, must be valid if there's at least one texture resource, the resource group will keep a reference to the sampler states
		*/
		ResourceGroup(Rhi::IRhi& rhi, uint32_t rootParameterIndex, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IResourceGroup(rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootParameterIndex(rootParameterIndex),
			mNumberOfResources(numberOfResources),
			mResources(RHI_MALLOC_TYPED(rhi.getContext(), Rhi::IResource*, mNumberOfResources)),
			mSamplerStates(nullptr)
		{
			// Process all resources and add our reference to the RHI resource
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex, ++resources)
			{
				Rhi::IResource* resource = *resources;
				RHI_ASSERT(rhi.getContext(), nullptr != resource, "Invalid null resource")
				mResources[resourceIndex] = resource;
				resource->addReference();
			}
			if (nullptr != samplerStates)
			{
				mSamplerStates = RHI_MALLOC_TYPED(rhi.getContext(), Rhi::ISamplerState*, mNumberOfResources);
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Rhi::ISamplerState* samplerState = mSamplerStates[resourceIndex] = samplerStates[resourceIndex];
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
			// Remove our reference from the RHI resources
			const Rhi::Context& context = getRhi().getContext();
			if (nullptr != mSamplerStates)
			{
				for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
				{
					Rhi::ISamplerState* samplerState = mSamplerStates[resourceIndex];
					if (nullptr != samplerState)
					{
						samplerState->releaseReference();
					}
				}
				RHI_FREE(context, mSamplerStates);
			}
			for (uint32_t resourceIndex = 0; resourceIndex < mNumberOfResources; ++resourceIndex)
			{
				mResources[resourceIndex]->releaseReference();
			}
			RHI_FREE(context, mResources);
		}

		/**
		*  @brief
		*    Return the number of resources this resource group groups together
		*
		*  @return
		*    The number of resources this resource group groups together
		*/
		[[nodiscard]] inline uint32_t getNumberOfResources() const
		{
			return mNumberOfResources;
		}

		/**
		*  @brief
		*    Return the RHI resources
		*
		*  @return
		*    The RHI resources, don't release or destroy the returned pointer
		*/
		[[nodiscard]] inline Rhi::IResource** getResources() const
		{
			return mResources;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ResourceGroup, this);
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
		uint32_t			 mRootParameterIndex;	///< The root parameter index number for binding
		uint32_t			 mNumberOfResources;	///< Number of resources this resource group groups together
		Rhi::IResource**	 mResources;			///< RHI resources, we keep a reference to it
		Rhi::ISamplerState** mSamplerStates;		///< Sampler states, we keep a reference to it


	};




	//[-------------------------------------------------------]
	//[ NullRhi/RootSignature.h                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null root signature ("pipeline layout" in Vulkan terminology) class
	*/
	class RootSignature final : public Rhi::IRootSignature
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] rootSignature
		*    Root signature to use
		*/
		RootSignature(NullRhi& nullRhi, const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IRootSignature(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootSignature(rootSignature)
		{
			const Rhi::Context& context = nullRhi.getContext();

			{ // Copy the parameter data
				const uint32_t numberOfParameters = mRootSignature.numberOfParameters;
				if (numberOfParameters > 0)
				{
					mRootSignature.parameters = RHI_MALLOC_TYPED(context, Rhi::RootParameter, numberOfParameters);
					Rhi::RootParameter* destinationRootParameters = const_cast<Rhi::RootParameter*>(mRootSignature.parameters);
					memcpy(destinationRootParameters, rootSignature.parameters, sizeof(Rhi::RootParameter) * numberOfParameters);

					// Copy the descriptor table data
					for (uint32_t i = 0; i < numberOfParameters; ++i)
					{
						Rhi::RootParameter& destinationRootParameter = destinationRootParameters[i];
						const Rhi::RootParameter& sourceRootParameter = rootSignature.parameters[i];
						if (Rhi::RootParameterType::DESCRIPTOR_TABLE == destinationRootParameter.parameterType)
						{
							const uint32_t numberOfDescriptorRanges = destinationRootParameter.descriptorTable.numberOfDescriptorRanges;
							destinationRootParameter.descriptorTable.descriptorRanges = reinterpret_cast<uintptr_t>(RHI_MALLOC_TYPED(context, Rhi::DescriptorRange, numberOfDescriptorRanges));
							memcpy(reinterpret_cast<Rhi::DescriptorRange*>(destinationRootParameter.descriptorTable.descriptorRanges), reinterpret_cast<const Rhi::DescriptorRange*>(sourceRootParameter.descriptorTable.descriptorRanges), sizeof(Rhi::DescriptorRange) * numberOfDescriptorRanges);
						}
					}
				}
			}

			{ // Copy the static sampler data
				const uint32_t numberOfStaticSamplers = mRootSignature.numberOfStaticSamplers;
				if (numberOfStaticSamplers > 0)
				{
					mRootSignature.staticSamplers = RHI_MALLOC_TYPED(context, Rhi::StaticSampler, numberOfStaticSamplers);
					memcpy(const_cast<Rhi::StaticSampler*>(mRootSignature.staticSamplers), rootSignature.staticSamplers, sizeof(Rhi::StaticSampler) * numberOfStaticSamplers);
				}
			}
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~RootSignature() override
		{
			const Rhi::Context& context = getRhi().getContext();
			if (nullptr != mRootSignature.parameters)
			{
				for (uint32_t i = 0; i < mRootSignature.numberOfParameters; ++i)
				{
					const Rhi::RootParameter& rootParameter = mRootSignature.parameters[i];
					if (Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType)
					{
						RHI_FREE(context, reinterpret_cast<Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges));
					}
				}
				RHI_FREE(context, const_cast<Rhi::RootParameter*>(mRootSignature.parameters));
			}
			RHI_FREE(context, const_cast<Rhi::StaticSampler*>(mRootSignature.staticSamplers));
		}

		/**
		*  @brief
		*    Return the root signature data
		*
		*  @return
		*    The root signature data
		*/
		[[nodiscard]] inline const Rhi::RootSignature& getRootSignature() const
		{
			return mRootSignature;
		}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRootSignature methods            ]
	//[-------------------------------------------------------]
	public:
		virtual Rhi::IResourceGroup* createResourceGroup(uint32_t rootParameterIndex, uint32_t numberOfResources, Rhi::IResource** resources, Rhi::ISamplerState** samplerStates = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			Rhi::IRhi& rhi = getRhi();

			// Sanity checks
			RHI_ASSERT(rhi.getContext(), rootParameterIndex < mRootSignature.numberOfParameters, "The null root parameter index is out-of-bounds")
			RHI_ASSERT(rhi.getContext(), numberOfResources > 0, "The number of null resources must not be zero")
			RHI_ASSERT(rhi.getContext(), nullptr != resources, "The null resource pointers must be valid")

			// Create resource group
			return RHI_NEW(rhi.getContext(), ResourceGroup)(rhi, rootParameterIndex, numberOfResources, resources, samplerStates RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), RootSignature, this);
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
		Rhi::RootSignature mRootSignature;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Buffer/VertexBuffer.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null vertex buffer object (VBO, "array buffer" in OpenGL terminology) class
	*/
	class VertexBuffer final : public Rhi::IVertexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit VertexBuffer(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IVertexBuffer(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), VertexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexBuffer(const VertexBuffer& source) = delete;
		VertexBuffer& operator =(const VertexBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Buffer/IndexBuffer.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null index buffer object (IBO, "element array buffer" in OpenGL terminology) class
	*/
	class IndexBuffer final : public Rhi::IIndexBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit IndexBuffer(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IIndexBuffer(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndexBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), IndexBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndexBuffer(const IndexBuffer& source) = delete;
		IndexBuffer& operator =(const IndexBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Buffer/VertexArray.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null vertex array class
	*/
	class VertexArray final : public Rhi::IVertexArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] id
		*    The unique compact vertex array ID
		*/
		inline explicit VertexArray(NullRhi& nullRhi, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IVertexArray(nullRhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexArray() override
		{
			// Free the unique compact vertex array ID
			static_cast<NullRhi&>(getRhi()).VertexArrayMakeId.DestroyID(getId());
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), VertexArray, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexArray(const VertexArray& source) = delete;
		VertexArray& operator =(const VertexArray& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Buffer/TextureBuffer.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null texture buffer object (TBO) class
	*/
	class TextureBuffer final : public Rhi::ITextureBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit TextureBuffer(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITextureBuffer(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TextureBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureBuffer(const TextureBuffer& source) = delete;
		TextureBuffer& operator =(const TextureBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Buffer/StructuredBuffer.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null structured buffer object (SBO) class
	*/
	class StructuredBuffer final : public Rhi::IStructuredBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit StructuredBuffer(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IStructuredBuffer(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~StructuredBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), StructuredBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit StructuredBuffer(const StructuredBuffer& source) = delete;
		StructuredBuffer& operator =(const StructuredBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Buffer/IndirectBuffer.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null indirect buffer object class
	*/
	class IndirectBuffer final : public Rhi::IIndirectBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit IndirectBuffer(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IIndirectBuffer(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~IndirectBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IIndirectBuffer methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const uint8_t* getEmulationData() const override
		{
			return nullptr;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), IndirectBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit IndirectBuffer(const IndirectBuffer& source) = delete;
		IndirectBuffer& operator =(const IndirectBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Buffer/UniformBuffer.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null uniform buffer object (UBO, "constant buffer" in Direct3D terminology) class
	*/
	class UniformBuffer final : public Rhi::IUniformBuffer
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit UniformBuffer(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IUniformBuffer(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~UniformBuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), UniformBuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit UniformBuffer(const UniformBuffer& source) = delete;
		UniformBuffer& operator =(const UniformBuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Buffer/BufferManager.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null buffer manager interface
	*/
	class BufferManager final : public Rhi::IBufferManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit BufferManager(NullRhi& nullRhi) :
			IBufferManager(nullRhi)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~BufferManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IBufferManager methods            ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Rhi::IVertexBuffer* createVertexBuffer([[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, [[maybe_unused]] Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), VertexBuffer)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IIndexBuffer* createIndexBuffer([[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = 0, [[maybe_unused]] Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, [[maybe_unused]] Rhi::IndexBufferFormat::Enum indexBufferFormat = Rhi::IndexBufferFormat::UNSIGNED_SHORT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), IndexBuffer)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::IVertexArray* createVertexArray([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, uint32_t numberOfVertexBuffers, const Rhi::VertexArrayVertexBuffer* vertexBuffers, Rhi::IIndexBuffer* indexBuffer = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity checks
			#ifdef RHI_DEBUG
			{
				const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
				for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
				{
					RHI_ASSERT(nullRhi.getContext(), &nullRhi == &vertexBuffer->vertexBuffer->getRhi(), "Null error: The given vertex buffer resource is owned by another RHI instance")
				}
			}
			#endif
			RHI_ASSERT(nullRhi.getContext(), nullptr == indexBuffer || &nullRhi == &indexBuffer->getRhi(), "Null error: The given index buffer resource is owned by another RHI instance")

			// We don't keep a reference to the vertex buffers used by the vertex array attributes in here
			// -> Ensure a correct reference counter behaviour
			const Rhi::VertexArrayVertexBuffer* vertexBufferEnd = vertexBuffers + numberOfVertexBuffers;
			for (const Rhi::VertexArrayVertexBuffer* vertexBuffer = vertexBuffers; vertexBuffer < vertexBufferEnd; ++vertexBuffer)
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
			uint16_t id = 0;
			return nullRhi.VertexArrayMakeId.CreateID(id) ? RHI_NEW(nullRhi.getContext(), VertexArray)(nullRhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER) : nullptr;
		}

		[[nodiscard]] inline virtual Rhi::ITextureBuffer* createTextureBuffer([[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t bufferFlags = Rhi::BufferFlag::SHADER_RESOURCE, [[maybe_unused]] Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat = Rhi::TextureFormat::R32G32B32A32F RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(nullRhi.getContext(), (numberOfBytes % Rhi::TextureFormat::getNumberOfBytesPerElement(textureFormat)) == 0, "The null texture buffer size must be a multiple of the selected texture format bytes per texel")

			// Create the texture buffer
			return RHI_NEW(nullRhi.getContext(), TextureBuffer)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IStructuredBuffer* createStructuredBuffer([[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t bufferFlags, [[maybe_unused]] Rhi::BufferUsage bufferUsage, [[maybe_unused]] uint32_t numberOfStructureBytes RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity checks
			RHI_ASSERT(nullRhi.getContext(), (numberOfBytes % numberOfStructureBytes) == 0, "The null structured buffer size must be a multiple of the given number of structure bytes")
			RHI_ASSERT(nullRhi.getContext(), (numberOfBytes % (sizeof(float) * 4)) == 0, "Performance: The null structured buffer should be aligned to a 128-bit stride, see \"Understanding Structured Buffer Performance\" by Evan Hart, posted Apr 17 2015 at 11:33AM - https://developer.nvidia.com/content/understanding-structured-buffer-performance")

			// Create the structured buffer
			return RHI_NEW(nullRhi.getContext(), StructuredBuffer)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IIndirectBuffer* createIndirectBuffer([[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t indirectBufferFlags = 0, [[maybe_unused]] Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity checks
			RHI_ASSERT(nullRhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 || (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0, "Invalid null flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" is missing")
			RHI_ASSERT(nullRhi.getContext(), !((indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) != 0 && (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) != 0), "Invalid null flags, indirect buffer element type specification \"DRAW_ARGUMENTS\" or \"DRAW_INDEXED_ARGUMENTS\" must be set, but not both at one and the same time")
			RHI_ASSERT(nullRhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawArguments)) == 0, "Null indirect buffer element type flags specification is \"DRAW_ARGUMENTS\" but the given number of bytes don't align to this")
			RHI_ASSERT(nullRhi.getContext(), (indirectBufferFlags & Rhi::IndirectBufferFlag::DRAW_INDEXED_ARGUMENTS) == 0 || (numberOfBytes % sizeof(Rhi::DrawIndexedArguments)) == 0, "Null indirect buffer element type flags specification is \"DRAW_INDEXED_ARGUMENTS\" but the given number of bytes don't align to this")

			// Create indirect buffer
			return RHI_NEW(nullRhi.getContext(), IndirectBuffer)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IUniformBuffer* createUniformBuffer([[maybe_unused]] uint32_t numberOfBytes, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] Rhi::BufferUsage bufferUsage = Rhi::BufferUsage::STATIC_DRAW RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Don't remove this reminder comment block: There are no buffer flags by intent since an uniform buffer can't be used for unordered access and as a consequence an uniform buffer must always used as shader resource to not be pointless
			// RHI_ASSERT(nullRhi.getContext(), (bufferFlags & Rhi::BufferFlag::UNORDERED_ACCESS) == 0, "Invalid null buffer flags, uniform buffer can't be used for unordered access")
			// RHI_ASSERT(nullRhi.getContext(), (bufferFlags & Rhi::BufferFlag::SHADER_RESOURCE) != 0, "Invalid null buffer flags, uniform buffer must be used as shader resource")

			// Create the uniform buffer
			return RHI_NEW(nullRhi.getContext(), UniformBuffer)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), BufferManager, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit BufferManager(const BufferManager& source) = delete;
		BufferManager& operator =(const BufferManager& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Texture/Texture1D.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null 1D texture class
	*/
	class Texture1D final : public Rhi::ITexture1D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] width
		*    The width of the texture
		*/
		inline Texture1D(NullRhi& nullRhi, uint32_t width RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture1D(nullRhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1D() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture1D, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1D(const Texture1D& source) = delete;
		Texture1D& operator =(const Texture1D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Texture/Texture1DArray.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null 1D array texture class
	*/
	class Texture1DArray final : public Rhi::ITexture1DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] numberOfSlices
		*    The number of slices
		*/
		inline Texture1DArray(NullRhi& nullRhi, uint32_t width, uint32_t numberOfSlices RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture1DArray(nullRhi, width, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture1DArray() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture1DArray, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture1DArray(const Texture1DArray& source) = delete;
		Texture1DArray& operator =(const Texture1DArray& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Texture/Texture2D.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null 2D texture class
	*/
	class Texture2D final : public Rhi::ITexture2D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*/
		inline Texture2D(NullRhi& nullRhi, uint32_t width, uint32_t height RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture2D(nullRhi, width, height RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2D() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture2D, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2D(const Texture2D& source) = delete;
		Texture2D& operator =(const Texture2D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Texture/Texture2DArray.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null 2D array texture class
	*/
	class Texture2DArray final : public Rhi::ITexture2DArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] numberOfSlices
		*    The number of slices
		*/
		inline Texture2DArray(NullRhi& nullRhi, uint32_t width, uint32_t height, uint32_t numberOfSlices RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture2DArray(nullRhi, width, height, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture2DArray() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture2DArray, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture2DArray(const Texture2DArray& source) = delete;
		Texture2DArray& operator =(const Texture2DArray& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Texture/Texture3D.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null 3D texture class
	*/
	class Texture3D final : public Rhi::ITexture3D
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] height
		*    The height of the texture
		*  @param[in] depth
		*    The depth of the texture
		*/
		inline Texture3D(NullRhi& nullRhi, uint32_t width, uint32_t height, uint32_t depth RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITexture3D(nullRhi, width, height, depth RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Texture3D() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Texture3D, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Texture3D(const Texture3D& source) = delete;
		Texture3D& operator =(const Texture3D& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Texture/TextureCube.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null cube texture class
	*/
	class TextureCube final : public Rhi::ITextureCube
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] width
		*    The width of the texture
		*/
		inline TextureCube(NullRhi& nullRhi, uint32_t width RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITextureCube(nullRhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureCube() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TextureCube, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureCube(const TextureCube& source) = delete;
		TextureCube& operator =(const TextureCube& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Texture/TextureCubeArray.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null cube array texture class
	*/
	class TextureCubeArray final : public Rhi::ITextureCubeArray
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] width
		*    The width of the texture
		*  @param[in] numberOfSlices
		*    The number of slices
		*/
		inline TextureCubeArray(NullRhi& nullRhi, uint32_t width, uint32_t numberOfSlices RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITextureCubeArray(nullRhi, width, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureCubeArray() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TextureCubeArray, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureCubeArray(const TextureCubeArray& source) = delete;
		TextureCubeArray& operator =(const TextureCubeArray& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Texture/TextureManager.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null texture manager interface
	*/
	class TextureManager final : public Rhi::ITextureManager
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit TextureManager(NullRhi& nullRhi) :
			ITextureManager(nullRhi)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TextureManager() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::ITextureManager methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] virtual Rhi::ITexture1D* createTexture1D(uint32_t width, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(nullRhi.getContext(), width > 0, "Null create texture 1D was called with invalid parameters")

			// Create 1D texture resource
			return RHI_NEW(nullRhi.getContext(), Texture1D)(nullRhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture1DArray* createTexture1DArray(uint32_t width, uint32_t numberOfSlices, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(nullRhi.getContext(), width > 0 && numberOfSlices > 0, "Null create texture 1D array was called with invalid parameters")

			// Create 1D texture array resource
			return RHI_NEW(nullRhi.getContext(), Texture1DArray)(nullRhi, width, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2D* createTexture2D(uint32_t width, uint32_t height, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT, [[maybe_unused]] uint8_t numberOfMultisamples = 1, [[maybe_unused]] const Rhi::OptimizedTextureClearValue* optimizedTextureClearValue = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(nullRhi.getContext(), width > 0 && height > 0, "Null create texture 2D was called with invalid parameters")

			// Create 2D texture resource
			return RHI_NEW(nullRhi.getContext(), Texture2D)(nullRhi, width, height RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture2DArray* createTexture2DArray(uint32_t width, uint32_t height, uint32_t numberOfSlices, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(nullRhi.getContext(), width > 0 && height > 0 && numberOfSlices > 0, "Null create texture 2D array was called with invalid parameters")

			// Create 2D texture array resource
			return RHI_NEW(nullRhi.getContext(), Texture2DArray)(nullRhi, width, height, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITexture3D* createTexture3D(uint32_t width, uint32_t height, uint32_t depth, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(nullRhi.getContext(), width > 0 && height > 0 && depth > 0, "Null create texture 3D was called with invalid parameters")

			// Create 3D texture resource
			return RHI_NEW(nullRhi.getContext(), Texture3D)(nullRhi, width, height, depth RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITextureCube* createTextureCube(uint32_t width, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(nullRhi.getContext(), width > 0, "Null create texture cube was called with invalid parameters")

			// Create cube texture resource
			return RHI_NEW(nullRhi.getContext(), TextureCube)(nullRhi, width RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::ITextureCubeArray* createTextureCubeArray(uint32_t width, uint32_t numberOfSlices, [[maybe_unused]] Rhi::TextureFormat::Enum textureFormat, [[maybe_unused]] const void* data = nullptr, [[maybe_unused]] uint32_t textureFlags = 0, [[maybe_unused]] Rhi::TextureUsage textureUsage = Rhi::TextureUsage::DEFAULT RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity check
			RHI_ASSERT(nullRhi.getContext(), width > 0, "Null create texture cube array was called with invalid parameters")

			// Create cube texture array resource
			return RHI_NEW(nullRhi.getContext(), TextureCubeArray)(nullRhi, width, numberOfSlices RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TextureManager, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TextureManager(const TextureManager& source) = delete;
		TextureManager& operator =(const TextureManager& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/State/SamplerState.h                          ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null sampler state class
	*/
	class SamplerState final : public Rhi::ISamplerState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit SamplerState(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ISamplerState(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SamplerState() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), SamplerState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit SamplerState(const SamplerState& source) = delete;
		SamplerState& operator =(const SamplerState& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/RenderTarget/RenderPass.h                     ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null render pass interface
	*/
	class RenderPass final : public Rhi::IRenderPass
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] rhi
		*    Owner RHI instance
		*  @param[in] numberOfColorAttachments
		*    Number of color render target textures, must be <="Rhi::Capabilities::maximumNumberOfSimultaneousRenderTargets"
		*  @param[in] colorAttachmentTextureFormats
		*    The color render target texture formats, can be a null pointer or can contain null pointers, if not a null pointer there must be at
		*    least "numberOfColorAttachments" textures in the provided C-array of pointers
		*  @param[in] depthStencilAttachmentTextureFormat
		*    The optional depth stencil render target texture format, can be a "Rhi::TextureFormat::UNKNOWN" if there should be no depth buffer
		*  @param[in] numberOfMultisamples
		*    The number of multisamples per pixel (valid values: 1, 2, 4, 8)
		*/
		RenderPass(Rhi::IRhi& rhi, uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IRenderPass(rhi RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNumberOfColorAttachments(numberOfColorAttachments),
			mDepthStencilAttachmentTextureFormat(depthStencilAttachmentTextureFormat),
			mNumberOfMultisamples(numberOfMultisamples)
		{
			RHI_ASSERT(rhi.getContext(), mNumberOfColorAttachments < 8, "Invalid number of null color attachments")
			memcpy(mColorAttachmentTextureFormats, colorAttachmentTextureFormats, sizeof(Rhi::TextureFormat::Enum) * mNumberOfColorAttachments);
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
		[[nodiscard]] inline uint32_t getNumberOfColorAttachments() const
		{
			return mNumberOfColorAttachments;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), RenderPass, this);
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
		uint32_t				 mNumberOfColorAttachments;
		Rhi::TextureFormat::Enum mColorAttachmentTextureFormats[8];
		Rhi::TextureFormat::Enum mDepthStencilAttachmentTextureFormat;
		uint8_t					 mNumberOfMultisamples;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/RenderTarget/SwapChain.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null swap chain class
	*/
	class SwapChain final : public Rhi::ISwapChain
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
		inline SwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ISwapChain(renderPass RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mNativeWindowHandle(windowHandle.nativeWindowHandle)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~SwapChain() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRenderTarget methods             ]
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
	//[ Public virtual Rhi::ISwapChain methods                ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual Rhi::handle getNativeWindowHandle() const override
		{
			return mNativeWindowHandle;
		}

		inline virtual void setVerticalSynchronizationInterval(uint32_t) override
		{}

		inline virtual void present() override
		{}

		inline virtual void resizeBuffers() override
		{}

		[[nodiscard]] inline virtual bool getFullscreenState() const override
		{
			// Window mode
			return false;
		}

		inline virtual void setFullscreenState(bool) override
		{}

		inline virtual void setRenderWindow(Rhi::IRenderWindow*) override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), SwapChain, this);
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
		Rhi::handle mNativeWindowHandle;	///< Native window handle window, can be a null handle


	};




	//[-------------------------------------------------------]
	//[ NullRhi/RenderTarget/Framebuffer.h                    ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null framebuffer class
	*/
	class Framebuffer final : public Rhi::IFramebuffer
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
		inline explicit Framebuffer(Rhi::IRenderPass& renderPass RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IFramebuffer(renderPass RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~Framebuffer() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRenderTarget methods             ]
	//[-------------------------------------------------------]
	public:
		inline virtual void getWidthAndHeight(uint32_t& width, uint32_t& height) const override
		{
			// TODO(co) Better implementation instead of just returning one (not that important, but would be nice)
			width  = 1;
			height = 1;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), Framebuffer, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit Framebuffer(const Framebuffer& source) = delete;
		Framebuffer& operator =(const Framebuffer& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/VertexShader.h                         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null vertex shader class
	*/
	class VertexShader final : public Rhi::IVertexShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit VertexShader(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IVertexShader(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~VertexShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), VertexShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit VertexShader(const VertexShader& source) = delete;
		VertexShader& operator =(const VertexShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/TessellationControlShader.h            ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null tessellation control shader ("hull shader" in Direct3D terminology) class
	*/
	class TessellationControlShader final : public Rhi::ITessellationControlShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit TessellationControlShader(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITessellationControlShader(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TessellationControlShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TessellationControlShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationControlShader(const TessellationControlShader& source) = delete;
		TessellationControlShader& operator =(const TessellationControlShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/TessellationEvaluationShader.h         ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null tessellation evaluation shader ("domain shader" in Direct3D terminology) class
	*/
	class TessellationEvaluationShader final : public Rhi::ITessellationEvaluationShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit TessellationEvaluationShader(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITessellationEvaluationShader(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TessellationEvaluationShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TessellationEvaluationShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TessellationEvaluationShader(const TessellationEvaluationShader& source) = delete;
		TessellationEvaluationShader& operator =(const TessellationEvaluationShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/GeometryShader.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null geometry shader class
	*/
	class GeometryShader final : public Rhi::IGeometryShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit GeometryShader(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGeometryShader(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~GeometryShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GeometryShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GeometryShader(const GeometryShader& source) = delete;
		GeometryShader& operator =(const GeometryShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/FragmentShader.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null fragment shader class (FS, "pixel shader" in Direct3D terminology)
	*/
	class FragmentShader final : public Rhi::IFragmentShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit FragmentShader(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IFragmentShader(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~FragmentShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), FragmentShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit FragmentShader(const FragmentShader& source) = delete;
		FragmentShader& operator =(const FragmentShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/TaskShader.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null task shader class (TS, "amplification shader" in Direct3D terminology)
	*/
	class TaskShader final : public Rhi::ITaskShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit TaskShader(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			ITaskShader(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~TaskShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), TaskShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit TaskShader(const TaskShader& source) = delete;
		TaskShader& operator =(const TaskShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/MeshShader.h                           ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null mesh shader class (MS)
	*/
	class MeshShader final : public Rhi::IMeshShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit MeshShader(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IMeshShader(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~MeshShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), MeshShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit MeshShader(const MeshShader& source) = delete;
		MeshShader& operator =(const MeshShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/ComputeShader.h                        ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null compute shader class (CS)
	*/
	class ComputeShader final : public Rhi::IComputeShader
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit ComputeShader(NullRhi& nullRhi RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IComputeShader(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ComputeShader() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShader methods                   ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ComputeShader, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputeShader(const ComputeShader& source) = delete;
		ComputeShader& operator =(const ComputeShader& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/GraphicsProgram.h                      ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null graphics program class
	*/
	class GraphicsProgram final : public Rhi::IGraphicsProgram
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor for traditional graphics program
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] vertexShader
		*    Vertex shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationControlShader
		*    Tessellation control shader the graphics program is using, can be a null pointer
		*  @param[in] tessellationEvaluationShader
		*    Tessellation evaluation shader the graphics program is using, can be a null pointer
		*  @param[in] geometryShader
		*    Geometry shader the graphics program is using, can be a null pointer
		*  @param[in] fragmentShader
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgram(NullRhi& nullRhi, VertexShader* vertexShader, TessellationControlShader* tessellationControlShader, TessellationEvaluationShader* tessellationEvaluationShader, GeometryShader* geometryShader, FragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGraphicsProgram(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
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
		*    Constructor for task and mesh shader based graphics program
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] taskShader
		*    Task shader the graphics program is using, can be a null pointer
		*  @param[in] meshShader
		*    Mesh shader the graphics program is using
		*  @param[in] fragmentShader
		*    Fragment shader the graphics program is using, can be a null pointer
		*
		*  @note
		*    - The graphics program keeps a reference to the provided shaders and releases it when no longer required
		*/
		GraphicsProgram(NullRhi& nullRhi, TaskShader* taskShader, MeshShader& meshShader, FragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGraphicsProgram(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER)
		{
			// We don't keep a reference to the shaders in here
			// -> Ensure a correct reference counter behaviour
			if (nullptr != taskShader)
			{
				taskShader->addReference();
				taskShader->releaseReference();
			}
			meshShader.addReference();
			meshShader.releaseReference();
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
		inline virtual ~GraphicsProgram() override
		{}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GraphicsProgram, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsProgram(const GraphicsProgram& source) = delete;
		GraphicsProgram& operator =(const GraphicsProgram& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/Shader/ShaderLanguage.h                       ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null shader language class
	*/
	class ShaderLanguage final : public Rhi::IShaderLanguage
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*/
		inline explicit ShaderLanguage(NullRhi& nullRhi) :
			IShaderLanguage(nullRhi)
		{}

		/**
		*  @brief
		*    Destructor
		*/
		inline virtual ~ShaderLanguage() override
		{}


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IShaderLanguage methods           ]
	//[-------------------------------------------------------]
	public:
		[[nodiscard]] inline virtual const char* getShaderLanguageName() const override
		{
			return ::detail::NULL_NAME;
		}

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromBytecode([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, [[maybe_unused]] const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::vertexShader", we know there's vertex shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), VertexShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IVertexShader* createVertexShaderFromSourceCode([[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, [[maybe_unused]] const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::vertexShader", we know there's vertex shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), VertexShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromBytecode([[maybe_unused]] const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), TessellationControlShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationControlShader* createTessellationControlShaderFromSourceCode([[maybe_unused]] const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation control shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), TessellationControlShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromBytecode([[maybe_unused]] const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), TessellationEvaluationShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITessellationEvaluationShader* createTessellationEvaluationShaderFromSourceCode([[maybe_unused]] const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::maximumNumberOfPatchVertices", we know there's tessellation evaluation shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), TessellationEvaluationShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromBytecode([[maybe_unused]] const Rhi::ShaderBytecode& shaderBytecode, [[maybe_unused]] Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), GeometryShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IGeometryShader* createGeometryShaderFromSourceCode([[maybe_unused]] const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::GsInputPrimitiveTopology gsInputPrimitiveTopology, [[maybe_unused]] Rhi::GsOutputPrimitiveTopology gsOutputPrimitiveTopology, [[maybe_unused]] uint32_t numberOfOutputVertices, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::maximumNumberOfGsOutputVertices", we know there's geometry shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), GeometryShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromBytecode([[maybe_unused]] const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::fragmentShader", we know there's fragment shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), FragmentShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IFragmentShader* createFragmentShaderFromSourceCode([[maybe_unused]] const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::fragmentShader", we know there's fragment shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), FragmentShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITaskShader* createTaskShaderFromBytecode([[maybe_unused]] const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::meshShader", we know there's mesh shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), TaskShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::ITaskShader* createTaskShaderFromSourceCode([[maybe_unused]] const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::meshShader", we know there's mesh shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), TaskShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IMeshShader* createMeshShaderFromBytecode([[maybe_unused]] const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::meshShader", we know there's mesh shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), MeshShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IMeshShader* createMeshShaderFromSourceCode([[maybe_unused]] const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::meshShader", we know there's mesh shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), MeshShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromBytecode([[maybe_unused]] const Rhi::ShaderBytecode& shaderBytecode RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::computeShader", we know there's compute shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), ComputeShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] inline virtual Rhi::IComputeShader* createComputeShaderFromSourceCode([[maybe_unused]] const Rhi::ShaderSourceCode& shaderSourceCode, [[maybe_unused]] Rhi::ShaderBytecode* shaderBytecode = nullptr RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			// There's no need to check for "Rhi::Capabilities::computeShader", we know there's compute shader support
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());
			return RHI_NEW(nullRhi.getContext(), ComputeShader)(nullRhi RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram([[maybe_unused]] const Rhi::IRootSignature& rootSignature, [[maybe_unused]] const Rhi::VertexAttributes& vertexAttributes, Rhi::IVertexShader* vertexShader, Rhi::ITessellationControlShader* tessellationControlShader, Rhi::ITessellationEvaluationShader* tessellationEvaluationShader, Rhi::IGeometryShader* geometryShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER) override
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			RHI_ASSERT(nullRhi.getContext(), nullptr == vertexShader || vertexShader->getShaderLanguageName() == ::detail::NULL_NAME, "Null vertex shader language mismatch")
			RHI_ASSERT(nullRhi.getContext(), nullptr == tessellationControlShader || tessellationControlShader->getShaderLanguageName() == ::detail::NULL_NAME, "Null tessellation control shader language mismatch")
			RHI_ASSERT(nullRhi.getContext(), nullptr == tessellationEvaluationShader || tessellationEvaluationShader->getShaderLanguageName() == ::detail::NULL_NAME, "Null tessellation evaluation shader language mismatch")
			RHI_ASSERT(nullRhi.getContext(), nullptr == geometryShader || geometryShader->getShaderLanguageName() == ::detail::NULL_NAME, "Null geometry shader language mismatch")
			RHI_ASSERT(nullRhi.getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::NULL_NAME, "Null fragment shader language mismatch")

			// Create the graphics program
			return RHI_NEW(nullRhi.getContext(), GraphicsProgram)(nullRhi, static_cast<VertexShader*>(vertexShader), static_cast<TessellationControlShader*>(tessellationControlShader), static_cast<TessellationEvaluationShader*>(tessellationEvaluationShader), static_cast<GeometryShader*>(geometryShader), static_cast<FragmentShader*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		[[nodiscard]] virtual Rhi::IGraphicsProgram* createGraphicsProgram([[maybe_unused]] const Rhi::IRootSignature& rootSignature, Rhi::ITaskShader* taskShader, Rhi::IMeshShader& meshShader, Rhi::IFragmentShader* fragmentShader RHI_RESOURCE_DEBUG_NAME_PARAMETER)
		{
			NullRhi& nullRhi = static_cast<NullRhi&>(getRhi());

			// Sanity checks
			// -> A shader can be a null pointer, but if it's not the shader and graphics program language must match
			// -> Optimization: Comparing the shader language name by directly comparing the pointer address of
			//    the name is safe because we know that we always reference to one and the same name address
			// TODO(co) Add security check: Is the given resource one of the currently used RHI?
			RHI_ASSERT(nullRhi.getContext(), nullptr == taskShader || taskShader->getShaderLanguageName() == ::detail::NULL_NAME, "Null task shader language mismatch")
			RHI_ASSERT(nullRhi.getContext(), meshShader.getShaderLanguageName() == ::detail::NULL_NAME, "Null mesh shader language mismatch")
			RHI_ASSERT(nullRhi.getContext(), nullptr == fragmentShader || fragmentShader->getShaderLanguageName() == ::detail::NULL_NAME, "Null fragment shader language mismatch")

			// Create the graphics program
			return RHI_NEW(nullRhi.getContext(), GraphicsProgram)(nullRhi, static_cast<TaskShader*>(taskShader), static_cast<MeshShader&>(meshShader), static_cast<FragmentShader*>(fragmentShader) RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ShaderLanguage, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ShaderLanguage(const ShaderLanguage& source) = delete;
		ShaderLanguage& operator =(const ShaderLanguage& source) = delete;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/State/GraphicsPipelineState.h                 ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null graphics pipeline state class
	*/
	class GraphicsPipelineState final : public Rhi::IGraphicsPipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] graphicsPipelineState
		*    Graphics pipeline state to use
		*  @param[in] id
		*    The unique compact graphics pipeline state ID
		*/
		GraphicsPipelineState(NullRhi& nullRhi, const Rhi::GraphicsPipelineState& graphicsPipelineState, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IGraphicsPipelineState(nullRhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootSignature(graphicsPipelineState.rootSignature),
			mGraphicsProgram(graphicsPipelineState.graphicsProgram),
			mRenderPass(graphicsPipelineState.renderPass)
		{
			// Add a reference to the referenced RHI resources
			mRootSignature->addReference();
			mGraphicsProgram->addReference();
			mRenderPass->addReference();
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~GraphicsPipelineState() override
		{
			// Release referenced RHI resources
			mRootSignature->releaseReference();
			mGraphicsProgram->releaseReference();
			mRenderPass->releaseReference();

			// Free the unique compact graphics pipeline state ID
			static_cast<NullRhi&>(getRhi()).GraphicsPipelineStateMakeId.DestroyID(getId());
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), GraphicsPipelineState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit GraphicsPipelineState(const GraphicsPipelineState& source) = delete;
		GraphicsPipelineState& operator =(const GraphicsPipelineState& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::IRootSignature*	mRootSignature;
		Rhi::IGraphicsProgram*	mGraphicsProgram;
		Rhi::IRenderPass*		mRenderPass;


	};




	//[-------------------------------------------------------]
	//[ NullRhi/State/ComputePipelineState.h                  ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Null compute pipeline state class
	*/
	class ComputePipelineState final : public Rhi::IComputePipelineState
	{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] nullRhi
		*    Owner null RHI instance
		*  @param[in] rootSignature
		*    Root signature to use
		*  @param[in] computeShader
		*    Compute shader to use
		*  @param[in] id
		*    The unique compact compute pipeline state ID
		*/
		ComputePipelineState(NullRhi& nullRhi, Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader, uint16_t id RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT) :
			IComputePipelineState(nullRhi, id RHI_RESOURCE_DEBUG_PASS_PARAMETER),
			mRootSignature(rootSignature),
			mComputeShader(computeShader)
		{
			// Add a reference to the given root signature and compute shader
			rootSignature.addReference();
			computeShader.addReference();
		}

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~ComputePipelineState() override
		{
			// Release the root signature and compute shader reference
			mRootSignature.releaseReference();
			mComputeShader.releaseReference();

			// Free the unique compact compute pipeline state ID
			static_cast<NullRhi&>(getRhi()).ComputePipelineStateMakeId.DestroyID(getId());
		}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	protected:
		inline virtual void selfDestruct() override
		{
			RHI_DELETE(getRhi().getContext(), ComputePipelineState, this);
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		explicit ComputePipelineState(const ComputePipelineState& source) = delete;
		ComputePipelineState& operator =(const ComputePipelineState& source) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		Rhi::IRootSignature& mRootSignature;
		Rhi::IComputeShader& mComputeShader;


	};




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // NullRhi




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
		namespace ImplementationDispatch
		{


			//[-------------------------------------------------------]
			//[ Command buffer                                        ]
			//[-------------------------------------------------------]
			void DispatchCommandBuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DispatchCommandBuffer* realData = static_cast<const Rhi::Command::DispatchCommandBuffer*>(data);
				RHI_ASSERT(rhi.getContext(), nullptr != realData->commandBufferToDispatch, "The null command buffer to dispatch must be valid")
				static_cast<NullRhi::NullRhi&>(rhi).dispatchCommandBufferInternal(*realData->commandBufferToDispatch);
			}

			//[-------------------------------------------------------]
			//[ Graphics                                              ]
			//[-------------------------------------------------------]
			void SetGraphicsRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsRootSignature* realData = static_cast<const Rhi::Command::SetGraphicsRootSignature*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setGraphicsRootSignature(realData->rootSignature);
			}

			void SetGraphicsPipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsPipelineState* realData = static_cast<const Rhi::Command::SetGraphicsPipelineState*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setGraphicsPipelineState(realData->graphicsPipelineState);
			}

			void SetGraphicsResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetGraphicsResourceGroup* realData = static_cast<const Rhi::Command::SetGraphicsResourceGroup*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setGraphicsResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void SetGraphicsVertexArray(const void* data, Rhi::IRhi& rhi)
			{
				// Input-assembler (IA) stage
				const Rhi::Command::SetGraphicsVertexArray* realData = static_cast<const Rhi::Command::SetGraphicsVertexArray*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setGraphicsVertexArray(realData->vertexArray);
			}

			void SetGraphicsViewports(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsViewports* realData = static_cast<const Rhi::Command::SetGraphicsViewports*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setGraphicsViewports(realData->numberOfViewports, (nullptr != realData->viewports) ? realData->viewports : reinterpret_cast<const Rhi::Viewport*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsScissorRectangles(const void* data, Rhi::IRhi& rhi)
			{
				// Rasterizer (RS) stage
				const Rhi::Command::SetGraphicsScissorRectangles* realData = static_cast<const Rhi::Command::SetGraphicsScissorRectangles*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setGraphicsScissorRectangles(realData->numberOfScissorRectangles, (nullptr != realData->scissorRectangles) ? realData->scissorRectangles : reinterpret_cast<const Rhi::ScissorRectangle*>(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData)));
			}

			void SetGraphicsRenderTarget(const void* data, Rhi::IRhi& rhi)
			{
				// Output-merger (OM) stage
				const Rhi::Command::SetGraphicsRenderTarget* realData = static_cast<const Rhi::Command::SetGraphicsRenderTarget*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setGraphicsRenderTarget(realData->renderTarget);
			}

			void ClearGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ClearGraphics* realData = static_cast<const Rhi::Command::ClearGraphics*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).clearGraphics(realData->clearFlags, realData->color, realData->z, realData->stencil);
			}

			void DrawGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawGraphics* realData = static_cast<const Rhi::Command::DrawGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<NullRhi::NullRhi&>(rhi).drawGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<NullRhi::NullRhi&>(rhi).drawGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawIndexedGraphics(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawIndexedGraphics* realData = static_cast<const Rhi::Command::DrawIndexedGraphics*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<NullRhi::NullRhi&>(rhi).drawIndexedGraphicsEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<NullRhi::NullRhi&>(rhi).drawIndexedGraphicsEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			void DrawMeshTasks(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DrawMeshTasks* realData = static_cast<const Rhi::Command::DrawMeshTasks*>(data);
				if (nullptr != realData->indirectBuffer)
				{
					// No resource owner security check in here, we only support emulated indirect buffer
					static_cast<NullRhi::NullRhi&>(rhi).drawMeshTasksEmulated(realData->indirectBuffer->getEmulationData(), realData->indirectBufferOffset, realData->numberOfDraws);
				}
				else
				{
					static_cast<NullRhi::NullRhi&>(rhi).drawMeshTasksEmulated(Rhi::CommandPacketHelper::getAuxiliaryMemory(realData), realData->indirectBufferOffset, realData->numberOfDraws);
				}
			}

			//[-------------------------------------------------------]
			//[ Compute                                               ]
			//[-------------------------------------------------------]
			void SetComputeRootSignature(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeRootSignature* realData = static_cast<const Rhi::Command::SetComputeRootSignature*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setComputeRootSignature(realData->rootSignature);
			}

			void SetComputePipelineState(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputePipelineState* realData = static_cast<const Rhi::Command::SetComputePipelineState*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setComputePipelineState(realData->computePipelineState);
			}

			void SetComputeResourceGroup(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::SetComputeResourceGroup* realData = static_cast<const Rhi::Command::SetComputeResourceGroup*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).setComputeResourceGroup(realData->rootParameterIndex, realData->resourceGroup);
			}

			void DispatchCompute(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::DispatchCompute* realData = static_cast<const Rhi::Command::DispatchCompute*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).dispatchCompute(realData->groupCountX, realData->groupCountY, realData->groupCountZ);
			}

			//[-------------------------------------------------------]
			//[ Resource                                              ]
			//[-------------------------------------------------------]
			void SetTextureMinimumMaximumMipmapIndex(const void*, Rhi::IRhi&)
			{}

			void ResolveMultisampleFramebuffer(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResolveMultisampleFramebuffer* realData = static_cast<const Rhi::Command::ResolveMultisampleFramebuffer*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).resolveMultisampleFramebuffer(*realData->destinationRenderTarget, *realData->sourceMultisampleFramebuffer);
			}

			void CopyResource(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::CopyResource* realData = static_cast<const Rhi::Command::CopyResource*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).copyResource(*realData->destinationResource, *realData->sourceResource);
			}

			void GenerateMipmaps(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::GenerateMipmaps* realData = static_cast<const Rhi::Command::GenerateMipmaps*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).generateMipmaps(*realData->resource);
			}

			//[-------------------------------------------------------]
			//[ Query                                                 ]
			//[-------------------------------------------------------]
			void ResetQueryPool(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::ResetQueryPool* realData = static_cast<const Rhi::Command::ResetQueryPool*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).resetQueryPool(*realData->queryPool, realData->firstQueryIndex, realData->numberOfQueries);
			}

			void BeginQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::BeginQuery* realData = static_cast<const Rhi::Command::BeginQuery*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).beginQuery(*realData->queryPool, realData->queryIndex, realData->queryControlFlags);
			}

			void EndQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::EndQuery* realData = static_cast<const Rhi::Command::EndQuery*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).endQuery(*realData->queryPool, realData->queryIndex);
			}

			void WriteTimestampQuery(const void* data, Rhi::IRhi& rhi)
			{
				const Rhi::Command::WriteTimestampQuery* realData = static_cast<const Rhi::Command::WriteTimestampQuery*>(data);
				static_cast<NullRhi::NullRhi&>(rhi).writeTimestampQuery(*realData->queryPool, realData->queryIndex);
			}

			//[-------------------------------------------------------]
			//[ Debug                                                 ]
			//[-------------------------------------------------------]
			#ifdef RHI_DEBUG
				void SetDebugMarker(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::SetDebugMarker* realData = static_cast<const Rhi::Command::SetDebugMarker*>(data);
					static_cast<NullRhi::NullRhi&>(rhi).setDebugMarker(realData->name);
				}

				void BeginDebugEvent(const void* data, Rhi::IRhi& rhi)
				{
					const Rhi::Command::BeginDebugEvent* realData = static_cast<const Rhi::Command::BeginDebugEvent*>(data);
					static_cast<NullRhi::NullRhi&>(rhi).beginDebugEvent(realData->name);
				}

				void EndDebugEvent(const void*, Rhi::IRhi& rhi)
				{
					static_cast<NullRhi::NullRhi&>(rhi).endDebugEvent();
				}
			#else
				void SetDebugMarker(const void*, Rhi::IRhi&)
				{}

				void BeginDebugEvent(const void*, Rhi::IRhi&)
				{}

				void EndDebugEvent(const void*, Rhi::IRhi&)
				{}
			#endif


		}


		//[-------------------------------------------------------]
		//[ Global definitions                                    ]
		//[-------------------------------------------------------]
		static constexpr Rhi::ImplementationDispatchFunction DISPATCH_FUNCTIONS[static_cast<uint8_t>(Rhi::CommandDispatchFunctionIndex::NUMBER_OF_FUNCTIONS)] =
		{
			// Command buffer
			&ImplementationDispatch::DispatchCommandBuffer,
			// Graphics
			&ImplementationDispatch::SetGraphicsRootSignature,
			&ImplementationDispatch::SetGraphicsPipelineState,
			&ImplementationDispatch::SetGraphicsResourceGroup,
			&ImplementationDispatch::SetGraphicsVertexArray,		// Input-assembler (IA) stage
			&ImplementationDispatch::SetGraphicsViewports,			// Rasterizer (RS) stage
			&ImplementationDispatch::SetGraphicsScissorRectangles,	// Rasterizer (RS) stage
			&ImplementationDispatch::SetGraphicsRenderTarget,		// Output-merger (OM) stage
			&ImplementationDispatch::ClearGraphics,
			&ImplementationDispatch::DrawGraphics,
			&ImplementationDispatch::DrawIndexedGraphics,
			&ImplementationDispatch::DrawMeshTasks,
			// Compute
			&ImplementationDispatch::SetComputeRootSignature,
			&ImplementationDispatch::SetComputePipelineState,
			&ImplementationDispatch::SetComputeResourceGroup,
			&ImplementationDispatch::DispatchCompute,
			// Resource
			&ImplementationDispatch::SetTextureMinimumMaximumMipmapIndex,
			&ImplementationDispatch::ResolveMultisampleFramebuffer,
			&ImplementationDispatch::CopyResource,
			&ImplementationDispatch::GenerateMipmaps,
			// Query
			&ImplementationDispatch::ResetQueryPool,
			&ImplementationDispatch::BeginQuery,
			&ImplementationDispatch::EndQuery,
			&ImplementationDispatch::WriteTimestampQuery,
			// Debug
			&ImplementationDispatch::SetDebugMarker,
			&ImplementationDispatch::BeginDebugEvent,
			&ImplementationDispatch::EndDebugEvent
		};


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}




//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace NullRhi
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	NullRhi::NullRhi(const Rhi::Context& context) :
		IRhi(Rhi::NameId::NULL_DUMMY, context),
		VertexArrayMakeId(context.getAllocator()),
		GraphicsPipelineStateMakeId(context.getAllocator()),
		ComputePipelineStateMakeId(context.getAllocator()),
		mShaderLanguage(nullptr),
		mRenderTarget(nullptr),
		mGraphicsRootSignature(nullptr),
		mComputeRootSignature(nullptr)
	{
		// Initialize the capabilities
		initializeCapabilities();
	}

	NullRhi::~NullRhi()
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
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->releaseReference();
			mComputeRootSignature = nullptr;
		}

		#ifdef RHI_STATISTICS
		{ // For debugging: At this point there should be no resource instances left, validate this!
			// -> Are the currently any resource instances?
			const uint32_t numberOfCurrentResources = getStatistics().getNumberOfCurrentResources();
			if (numberOfCurrentResources > 0)
			{
				// Error!
				if (numberOfCurrentResources > 1)
				{
					RHI_ASSERT(mContext, false, "The null RHI implementation is going to be destroyed, but there are still %u resource instances left (memory leak)", numberOfCurrentResources)
				}
				else
				{
					RHI_ASSERT(mContext, false, "The null RHI implementation is going to be destroyed, but there is still one resource instance left (memory leak)")
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

	void NullRhi::dispatchCommandBufferInternal(const Rhi::CommandBuffer& commandBuffer)
	{
		// Loop through all commands
		const uint8_t* commandPacketBuffer = commandBuffer.getCommandPacketBuffer();
		Rhi::ConstCommandPacket constCommandPacket = commandPacketBuffer;
		while (nullptr != constCommandPacket)
		{
			{ // Dispatch command packet
				const Rhi::CommandDispatchFunctionIndex commandDispatchFunctionIndex = Rhi::CommandPacketHelper::loadCommandDispatchFunctionIndex(constCommandPacket);
				const void* command = Rhi::CommandPacketHelper::loadCommand(constCommandPacket);
				detail::DISPATCH_FUNCTIONS[static_cast<uint32_t>(commandDispatchFunctionIndex)](command, *this);
			}

			{ // Next command
				const uint32_t nextCommandPacketByteIndex = Rhi::CommandPacketHelper::getNextCommandPacketByteIndex(constCommandPacket);
				constCommandPacket = (~0u != nextCommandPacketByteIndex) ? &commandPacketBuffer[nextCommandPacketByteIndex] : nullptr;
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Graphics                                              ]
	//[-------------------------------------------------------]
	void NullRhi::setGraphicsRootSignature(Rhi::IRootSignature* rootSignature)
	{
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->releaseReference();
		}
		mGraphicsRootSignature = static_cast<RootSignature*>(rootSignature);
		if (nullptr != mGraphicsRootSignature)
		{
			mGraphicsRootSignature->addReference();

			// Sanity check
			RHI_MATCH_CHECK(*this, *rootSignature)
		}
	}

	void NullRhi::setGraphicsPipelineState(Rhi::IGraphicsPipelineState* graphicsPipelineState)
	{
		if (nullptr != graphicsPipelineState)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *graphicsPipelineState)
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void NullRhi::setGraphicsResourceGroup([[maybe_unused]] uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			RHI_ASSERT(mContext, nullptr != mGraphicsRootSignature, "No null RHI implementation graphics root signature set")
			const Rhi::RootSignature& rootSignature = mGraphicsRootSignature->getRootSignature();
			RHI_ASSERT(mContext, rootParameterIndex < rootSignature.numberOfParameters, "The null RHI implementation root parameter index is out of bounds")
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			RHI_ASSERT(mContext, Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType, "The null RHI implementation root parameter index doesn't reference a descriptor table")
			RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "The null RHI implementation descriptor ranges is a null pointer")
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// TODO(co) Some additional resource type root signature security checks in debug build?
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void NullRhi::setGraphicsVertexArray(Rhi::IVertexArray* vertexArray)
	{
		// Input-assembler (IA) stage

		// Nothing here, the following is just for debugging
		if (nullptr != vertexArray)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *vertexArray)
		}
	}

	void NullRhi::setGraphicsViewports([[maybe_unused]] uint32_t numberOfViewports, [[maybe_unused]] const Rhi::Viewport* viewports)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfViewports > 0 && nullptr != viewports, "Invalid null rasterizer state viewports")
	}

	void NullRhi::setGraphicsScissorRectangles([[maybe_unused]] uint32_t numberOfScissorRectangles, [[maybe_unused]] const Rhi::ScissorRectangle* scissorRectangles)
	{
		// Rasterizer (RS) stage

		// Sanity check
		RHI_ASSERT(mContext, numberOfScissorRectangles > 0 && nullptr != scissorRectangles, "Invalid null rasterizer state scissor rectangles")
	}

	void NullRhi::setGraphicsRenderTarget(Rhi::IRenderTarget* renderTarget)
	{
		// Output-merger (OM) stage

		// New render target?
		if (mRenderTarget != renderTarget)
		{
			// Set a render target?
			if (nullptr != renderTarget)
			{
				// Sanity check
				RHI_MATCH_CHECK(*this, *renderTarget)

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

	void NullRhi::clearGraphics(uint32_t, const float [4], [[maybe_unused]] float z, uint32_t)
	{
		// Sanity check
		RHI_ASSERT(mContext, z >= 0.0f && z <= 1.0f, "The null clear graphics z value must be between [0, 1] (inclusive)")
	}

	void NullRhi::drawGraphicsEmulated([[maybe_unused]] const uint8_t* emulationData, uint32_t, [[maybe_unused]] uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The null emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of null draws must not be zero")
	}

	void NullRhi::drawIndexedGraphicsEmulated([[maybe_unused]] const uint8_t* emulationData, uint32_t, [[maybe_unused]] uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The null emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of null draws must not be zero")
	}

	void NullRhi::drawMeshTasksEmulated([[maybe_unused]] const uint8_t* emulationData, uint32_t, [[maybe_unused]] uint32_t numberOfDraws)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != emulationData, "The null emulation data must be valid")
		RHI_ASSERT(mContext, numberOfDraws > 0, "The number of null draws must not be zero")
	}


	//[-------------------------------------------------------]
	//[ Compute                                               ]
	//[-------------------------------------------------------]
	void NullRhi::setComputeRootSignature(Rhi::IRootSignature* rootSignature)
	{
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->releaseReference();
		}
		mComputeRootSignature = static_cast<RootSignature*>(rootSignature);
		if (nullptr != mComputeRootSignature)
		{
			mComputeRootSignature->addReference();

			// Sanity check
			RHI_MATCH_CHECK(*this, *rootSignature)
		}
	}

	void NullRhi::setComputePipelineState(Rhi::IComputePipelineState* computePipelineState)
	{
		if (nullptr != computePipelineState)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *computePipelineState)
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void NullRhi::setComputeResourceGroup([[maybe_unused]] uint32_t rootParameterIndex, Rhi::IResourceGroup* resourceGroup)
	{
		// Security checks
		#ifdef RHI_DEBUG
		{
			RHI_ASSERT(mContext, nullptr != mComputeRootSignature, "No null RHI implementation compute root signature set")
			const Rhi::RootSignature& rootSignature = mComputeRootSignature->getRootSignature();
			RHI_ASSERT(mContext, rootParameterIndex < rootSignature.numberOfParameters, "The null RHI implementation root parameter index is out of bounds")
			const Rhi::RootParameter& rootParameter = rootSignature.parameters[rootParameterIndex];
			RHI_ASSERT(mContext, Rhi::RootParameterType::DESCRIPTOR_TABLE == rootParameter.parameterType, "The null RHI implementation root parameter index doesn't reference a descriptor table")
			RHI_ASSERT(mContext, nullptr != reinterpret_cast<const Rhi::DescriptorRange*>(rootParameter.descriptorTable.descriptorRanges), "The null RHI implementation descriptor ranges is a null pointer")
		}
		#endif

		if (nullptr != resourceGroup)
		{
			// Sanity check
			RHI_MATCH_CHECK(*this, *resourceGroup)

			// TODO(co) Some additional resource type root signature security checks in debug build?
		}
		else
		{
			// TODO(co) Handle this situation?
		}
	}

	void NullRhi::dispatchCompute(uint32_t, uint32_t, uint32_t)
	{}


	//[-------------------------------------------------------]
	//[ Resource                                              ]
	//[-------------------------------------------------------]
	void NullRhi::resolveMultisampleFramebuffer(Rhi::IRenderTarget&, Rhi::IFramebuffer&)
	{}

	void NullRhi::copyResource(Rhi::IResource&, Rhi::IResource&)
	{}

	void NullRhi::generateMipmaps(Rhi::IResource&)
	{}


	//[-------------------------------------------------------]
	//[ Query                                                 ]
	//[-------------------------------------------------------]
	void NullRhi::resetQueryPool(Rhi::IQueryPool&, uint32_t, uint32_t)
	{}

	void NullRhi::beginQuery(Rhi::IQueryPool&, uint32_t, uint32_t)
	{}

	void NullRhi::endQuery(Rhi::IQueryPool&, uint32_t)
	{}

	void NullRhi::writeTimestampQuery(Rhi::IQueryPool&, uint32_t)
	{}

	//[-------------------------------------------------------]
	//[ Debug                                                 ]
	//[-------------------------------------------------------]
	#ifdef RHI_DEBUG
		void NullRhi::setDebugMarker(const char*)
		{}

		void NullRhi::beginDebugEvent(const char*)
		{}

		void NullRhi::endDebugEvent()
		{}
	#endif


	//[-------------------------------------------------------]
	//[ Public virtual Rhi::IRhi methods                      ]
	//[-------------------------------------------------------]
	const char* NullRhi::getName() const
	{
		return "Null";
	}

	bool NullRhi::isInitialized() const
	{
		return true;
	}

	bool NullRhi::isDebugEnabled()
	{
		// Nothing here

		// Debug disabled
		return false;
	}


	//[-------------------------------------------------------]
	//[ Shader language                                       ]
	//[-------------------------------------------------------]
	uint32_t NullRhi::getNumberOfShaderLanguages() const
	{
		// Only one shader language supported in here
		return 1;
	}

	const char* NullRhi::getShaderLanguageName([[maybe_unused]] uint32_t index) const
	{
		RHI_ASSERT(mContext, index < getNumberOfShaderLanguages(), "Null: Shader language index is out-of-bounds")
		return ::detail::NULL_NAME;
	}

	Rhi::IShaderLanguage* NullRhi::getShaderLanguage(const char* shaderLanguageName)
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
					mShaderLanguage = RHI_NEW(mContext, ShaderLanguage)(*this);
					mShaderLanguage->addReference();	// Internal RHI reference
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
	Rhi::IRenderPass* NullRhi::createRenderPass(uint32_t numberOfColorAttachments, const Rhi::TextureFormat::Enum* colorAttachmentTextureFormats, Rhi::TextureFormat::Enum depthStencilAttachmentTextureFormat, uint8_t numberOfMultisamples RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RenderPass)(*this, numberOfColorAttachments, colorAttachmentTextureFormats, depthStencilAttachmentTextureFormat, numberOfMultisamples RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IQueryPool* NullRhi::createQueryPool([[maybe_unused]] Rhi::QueryType queryType, [[maybe_unused]] uint32_t numberOfQueries RHI_RESOURCE_DEBUG_NAME_MAYBE_UNUSED_PARAMETER_NO_DEFAULT)
	{
		// TODO(co) Implement me
		return nullptr;
	}

	Rhi::ISwapChain* NullRhi::createSwapChain(Rhi::IRenderPass& renderPass, Rhi::WindowHandle windowHandle, bool RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, renderPass)
		RHI_ASSERT(mContext, NULL_HANDLE != windowHandle.nativeWindowHandle, "Null: The provided native window handle must not be a null handle")

		// Create the swap chain
		return RHI_NEW(mContext, SwapChain)(renderPass, windowHandle RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IFramebuffer* NullRhi::createFramebuffer(Rhi::IRenderPass& renderPass, const Rhi::FramebufferAttachment* colorFramebufferAttachments, const Rhi::FramebufferAttachment* depthStencilFramebufferAttachment RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity check
		RHI_MATCH_CHECK(*this, renderPass)

		// We don't keep a reference to the provided textures in here
		// -> Ensure a correct reference counter behaviour

		// Are there any color textures?
		const uint32_t numberOfColorAttachments = static_cast<RenderPass&>(renderPass).getNumberOfColorAttachments();
		if (numberOfColorAttachments > 0)
		{
			// Loop through all color textures
			const Rhi::FramebufferAttachment* colorFramebufferAttachmentsEnd = colorFramebufferAttachments + numberOfColorAttachments;
			for (const Rhi::FramebufferAttachment* colorFramebufferAttachment = colorFramebufferAttachments; colorFramebufferAttachment < colorFramebufferAttachmentsEnd; ++colorFramebufferAttachment)
			{
				// Valid entry?
				if (nullptr != colorFramebufferAttachment->texture)
				{
					// TODO(co) Add security check: Is the given resource one of the currently used RHI?
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
		return RHI_NEW(mContext, Framebuffer)(renderPass RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IBufferManager* NullRhi::createBufferManager()
	{
		return RHI_NEW(mContext, BufferManager)(*this);
	}

	Rhi::ITextureManager* NullRhi::createTextureManager()
	{
		return RHI_NEW(mContext, TextureManager)(*this);
	}

	Rhi::IRootSignature* NullRhi::createRootSignature(const Rhi::RootSignature& rootSignature RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, RootSignature)(*this, rootSignature RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}

	Rhi::IGraphicsPipelineState* NullRhi::createGraphicsPipelineState(const Rhi::GraphicsPipelineState& graphicsPipelineState RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.rootSignature, "Null: Invalid graphics pipeline state root signature")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.graphicsProgram, "Null: Invalid graphics pipeline state graphics program")
		RHI_ASSERT(mContext, nullptr != graphicsPipelineState.renderPass, "Null: Invalid graphics pipeline state render pass")

		// Create graphics pipeline state
		uint16_t id = 0;
		if (GraphicsPipelineStateMakeId.CreateID(id))
		{
			return RHI_NEW(mContext, GraphicsPipelineState)(*this, graphicsPipelineState, id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		// Error: Ensure a correct reference counter behaviour
		graphicsPipelineState.rootSignature->addReference();
		graphicsPipelineState.rootSignature->releaseReference();
		graphicsPipelineState.graphicsProgram->addReference();
		graphicsPipelineState.graphicsProgram->releaseReference();
		graphicsPipelineState.renderPass->addReference();
		graphicsPipelineState.renderPass->releaseReference();
		return nullptr;
	}

	Rhi::IComputePipelineState* NullRhi::createComputePipelineState(Rhi::IRootSignature& rootSignature, Rhi::IComputeShader& computeShader RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		// Sanity checks
		RHI_MATCH_CHECK(*this, rootSignature)
		RHI_MATCH_CHECK(*this, computeShader)

		// Create the compute pipeline state
		uint16_t id = 0;
		if (ComputePipelineStateMakeId.CreateID(id))
		{
			return RHI_NEW(mContext, ComputePipelineState)(*this, rootSignature, computeShader, id RHI_RESOURCE_DEBUG_PASS_PARAMETER);
		}

		// Error: Ensure a correct reference counter behaviour
		rootSignature.addReference();
		rootSignature.releaseReference();
		computeShader.addReference();
		computeShader.releaseReference();
		return nullptr;
	}

	Rhi::ISamplerState* NullRhi::createSamplerState(const Rhi::SamplerState& RHI_RESOURCE_DEBUG_NAME_PARAMETER_NO_DEFAULT)
	{
		return RHI_NEW(mContext, SamplerState)(*this RHI_RESOURCE_DEBUG_PASS_PARAMETER);
	}


	//[-------------------------------------------------------]
	//[ Resource handling                                     ]
	//[-------------------------------------------------------]
	bool NullRhi::map(Rhi::IResource&, uint32_t, Rhi::MapType, uint32_t, Rhi::MappedSubresource&)
	{
		// Not supported by the null RHI
		return false;
	}

	void NullRhi::unmap(Rhi::IResource&, uint32_t)
	{}

	bool NullRhi::getQueryPoolResults(Rhi::IQueryPool&, uint32_t, uint8_t*, uint32_t, uint32_t, uint32_t, uint32_t)
	{
		return true;
	}


	//[-------------------------------------------------------]
	//[ Operation                                             ]
	//[-------------------------------------------------------]
	void NullRhi::dispatchCommandBuffer(const Rhi::CommandBuffer& commandBuffer)
	{
		// Sanity check
		RHI_ASSERT(mContext, !commandBuffer.isEmpty(), "The null command buffer to dispatch mustn't be empty")

		// Dispatch command buffer
		dispatchCommandBufferInternal(commandBuffer);
	}


	//[-------------------------------------------------------]
	//[ Protected virtual Rhi::RefCount methods               ]
	//[-------------------------------------------------------]
	void NullRhi::selfDestruct()
	{
		RHI_DELETE(mContext, NullRhi, this);
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void NullRhi::initializeCapabilities()
	{
		strcpy(mCapabilities.deviceName, "Null");

		// Preferred swap chain texture format
		mCapabilities.preferredSwapChainColorTextureFormat		  = Rhi::TextureFormat::Enum::R8G8B8A8;
		mCapabilities.preferredSwapChainDepthStencilTextureFormat = Rhi::TextureFormat::Enum::D32_FLOAT;

		// Maximum number of viewports (always at least 1)
		mCapabilities.maximumNumberOfViewports = 1;

		// Maximum number of simultaneous render targets (if <1 render to texture is not supported)
		mCapabilities.maximumNumberOfSimultaneousRenderTargets = 8;

		// Maximum texture dimension
		mCapabilities.maximumTextureDimension = 42;

		// Maximum number of 1D texture array slices (usually 512, in case there's no support for 1D texture arrays it's 0)
		mCapabilities.maximumNumberOf1DTextureArraySlices = 42;

		// Maximum number of 2D texture array slices (usually 512, in case there's no support for 2D texture arrays it's 0)
		mCapabilities.maximumNumberOf2DTextureArraySlices = 42;

		// Maximum number of cube texture array slices (usually 512, in case there's no support for cube texture arrays it's 0)
		mCapabilities.maximumNumberOfCubeTextureArraySlices = 42;

		// Maximum texture buffer (TBO) size in texel (>65536, typically much larger than that of one-dimensional texture, in case there's no support for texture buffer it's 0)
		mCapabilities.maximumTextureBufferSize = mCapabilities.maximumStructuredBufferSize = 42;

		// Maximum indirect buffer size in bytes
		mCapabilities.maximumIndirectBufferSize = 128 * 1024;	// 128 KiB

		// Maximum uniform buffer (UBO) size in bytes (usually at least 4096 * 16 bytes, in case there's no support for uniform buffer it's 0)
		// -> Let's use the DirectX 11 value: See https://msdn.microsoft.com/en-us/library/windows/desktop/ff819065(v=vs.85).aspx - "Resource Limits (Direct3D 11)" - "Number of elements in a constant buffer D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT (4096)"
		// -> One element = float4 = 16 bytes
		mCapabilities.maximumUniformBufferSize = 4096 * 16;

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

		// The null RHI has native multithreading
		mCapabilities.nativeMultithreading = true;

		// The null RHI has no shader bytecode support
		mCapabilities.shaderBytecode = false;

		// Is there support for vertex shaders (VS)?
		mCapabilities.vertexShader = true;

		// Maximum number of vertices per patch (usually 0 for no tessellation support or 32 which is the maximum number of supported vertices per patch)
		mCapabilities.maximumNumberOfPatchVertices = 32;

		// Maximum number of vertices a geometry shader can emit (usually 0 for no geometry shader support or 1024)
		mCapabilities.maximumNumberOfGsOutputVertices = 1024;

		// Is there support for fragment shaders (FS)?
		mCapabilities.fragmentShader = true;

		// Is there support for task shaders (TS) and mesh shaders (MS)?
		mCapabilities.meshShader = true;

		// Is there support for compute shaders (CS)?
		mCapabilities.computeShader = true;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // NullRhi




//[-------------------------------------------------------]
//[ Global functions                                      ]
//[-------------------------------------------------------]
// Export the instance creation function
#ifdef RHI_NULL_EXPORTS
	#define NULLRHI_FUNCTION_EXPORT GENERIC_FUNCTION_EXPORT
#else
	#define NULLRHI_FUNCTION_EXPORT
#endif
NULLRHI_FUNCTION_EXPORT Rhi::IRhi* createNullRhiInstance(const Rhi::Context& context)
{
	return RHI_NEW(context, NullRhi::NullRhi)(context);
}
#undef NULLRHI_FUNCTION_EXPORT
