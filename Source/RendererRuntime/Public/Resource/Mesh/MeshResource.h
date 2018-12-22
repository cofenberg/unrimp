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
#include "RendererRuntime/Public/Resource/IResource.h"
#include "RendererRuntime/Public/Resource/Mesh/SubMesh.h"

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4127)	// warning C4127: conditional expression is constant
	PRAGMA_WARNING_DISABLE_MSVC(4201)	// warning C4201: nonstandard extension used: nameless struct/union
	PRAGMA_WARNING_DISABLE_MSVC(4464)	// warning C4464: relative include path contains '..'
	PRAGMA_WARNING_DISABLE_MSVC(4324)	// warning C4324: '<x>': structure was padded due to alignment specifier
	#include <glm/glm.hpp>
PRAGMA_WARNING_POP

// Disable warnings in external headers, we can't fix them
PRAGMA_WARNING_PUSH
	PRAGMA_WARNING_DISABLE_MSVC(4365)	// warning C4365: 'argument': conversion from 'long' to 'unsigned int', signed/unsigned mismatch
	PRAGMA_WARNING_DISABLE_MSVC(4571)	// warning C4571: Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
	PRAGMA_WARNING_DISABLE_MSVC(4668)	// warning C4668: '_M_HYBRID_X86_ARM64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
	#include <vector>
PRAGMA_WARNING_POP


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	template <class ELEMENT_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class PackedElementManager;
	template <class TYPE, class LOADER_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class ResourceManagerTemplate;
	class IMeshResourceLoader;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef std::vector<SubMesh> SubMeshes;
	typedef uint32_t			 MeshResourceId;		///< POD mesh resource identifier
	typedef uint32_t			 SkeletonResourceId;	///< POD skeleton resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Mesh resource class
	*/
	class MeshResource final : public IResource
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend PackedElementManager<MeshResource, MeshResourceId, 4096>;							// Type definition of template class
		friend ResourceManagerTemplate<MeshResource, IMeshResourceLoader, MeshResourceId, 4096>;	// Type definition of template class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		RENDERERRUNTIME_API_EXPORT static const Renderer::VertexAttributes VERTEX_ATTRIBUTES;			///< Default vertex attributes layout, whenever possible stick to this to be as compatible as possible to the rest
		RENDERERRUNTIME_API_EXPORT static const Renderer::VertexAttributes SKINNED_VERTEX_ATTRIBUTES;	///< Default skinned vertex attributes layout, whenever possible stick to this to be as compatible as possible to the rest


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		//[-------------------------------------------------------]
		//[ Bounding                                              ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline const glm::vec3& getMinimumBoundingBoxPosition() const
		{
			return mMinimumBoundingBoxPosition;
		}

		[[nodiscard]] inline const glm::vec3& getMaximumBoundingBoxPosition() const
		{
			return mMaximumBoundingBoxPosition;
		}

		inline void setBoundingBoxPosition(const glm::vec3& minimumBoundingBoxPosition, const glm::vec3& maximumBoundingBoxPosition)
		{
			mMinimumBoundingBoxPosition = minimumBoundingBoxPosition;
			mMaximumBoundingBoxPosition = maximumBoundingBoxPosition;
		}

		[[nodiscard]] inline const glm::vec3& getBoundingSpherePosition() const
		{
			return mBoundingSpherePosition;
		}

		[[nodiscard]] inline float getBoundingSphereRadius() const
		{
			return mBoundingSphereRadius;
		}

		inline void setBoundingSpherePositionRadius(const glm::vec3& boundingSpherePosition, float boundingSphereRadius)
		{
			mBoundingSpherePosition = boundingSpherePosition;
			mBoundingSphereRadius = boundingSphereRadius;
		}

		//[-------------------------------------------------------]
		//[ Vertex and index data                                 ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline uint32_t getNumberOfVertices() const
		{
			return mNumberOfVertices;
		}

		inline void setNumberOfVertices(uint32_t numberOfVertices)
		{
			mNumberOfVertices = numberOfVertices;
		}

		[[nodiscard]] inline uint32_t getNumberOfIndices() const
		{
			return mNumberOfIndices;
		}

		inline void setNumberOfIndices(uint32_t numberOfIndices)
		{
			mNumberOfIndices = numberOfIndices;
		}

		[[nodiscard]] inline const Renderer::IVertexArrayPtr& getVertexArrayPtr() const
		{
			return mVertexArray;
		}

		inline void setVertexArray(Renderer::IVertexArray* vertexArray)
		{
			mVertexArray = vertexArray;
		}

		//[-------------------------------------------------------]
		//[ Sub-meshes                                            ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline const SubMeshes& getSubMeshes() const
		{
			return mSubMeshes;
		}

		[[nodiscard]] inline SubMeshes& getSubMeshes()
		{
			return mSubMeshes;
		}

		//[-------------------------------------------------------]
		//[ Optional skeleton                                     ]
		//[-------------------------------------------------------]
		[[nodiscard]] inline SkeletonResourceId getSkeletonResourceId() const
		{
			return mSkeletonResourceId;
		}

		inline void setSkeletonResourceId(SkeletonResourceId skeletonResourceId)
		{
			mSkeletonResourceId = skeletonResourceId;
		}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline MeshResource() :
			// Bounding
			mMinimumBoundingBoxPosition(getInvalid<float>()),
			mMaximumBoundingBoxPosition(getInvalid<float>()),
			mBoundingSpherePosition(getInvalid<float>()),
			mBoundingSphereRadius(getInvalid<float>()),
			// Vertex and index data
			mNumberOfVertices(0),
			mNumberOfIndices(0),
			// Optional skeleton
			mSkeletonResourceId(getInvalid<SkeletonResourceId>())
		{
			// Nothing here
		}

		inline virtual ~MeshResource() override
		{
			// Sanity checks
			assert(isInvalid(mMinimumBoundingBoxPosition.x) && isInvalid(mMinimumBoundingBoxPosition.y) && isInvalid(mMinimumBoundingBoxPosition.z));
			assert(isInvalid(mMaximumBoundingBoxPosition.x) && isInvalid(mMaximumBoundingBoxPosition.y) && isInvalid(mMaximumBoundingBoxPosition.z));
			assert(isInvalid(mBoundingSpherePosition.x) && isInvalid(mBoundingSpherePosition.y) && isInvalid(mBoundingSpherePosition.z));
			assert(isInvalid(mBoundingSphereRadius));
			assert(0 == mNumberOfVertices);
			assert(0 == mNumberOfIndices);
			assert(nullptr == mVertexArray.getPointer());
			assert(mSubMeshes.empty());
			assert(isInvalid(mSkeletonResourceId));
		}

		explicit MeshResource(const MeshResource&) = delete;
		MeshResource& operator=(const MeshResource&) = delete;

		//[-------------------------------------------------------]
		//[ "RendererRuntime::PackedElementManager" management    ]
		//[-------------------------------------------------------]
		inline void initializeElement(MeshResourceId meshResourceId)
		{
			// Sanity checks
			assert(isInvalid(mMinimumBoundingBoxPosition.x) && isInvalid(mMinimumBoundingBoxPosition.y) && isInvalid(mMinimumBoundingBoxPosition.z));
			assert(isInvalid(mMaximumBoundingBoxPosition.x) && isInvalid(mMaximumBoundingBoxPosition.y) && isInvalid(mMaximumBoundingBoxPosition.z));
			assert(isInvalid(mBoundingSpherePosition.x) && isInvalid(mBoundingSpherePosition.y) && isInvalid(mBoundingSpherePosition.z));
			assert(isInvalid(mBoundingSphereRadius));
			assert(0 == mNumberOfVertices);
			assert(0 == mNumberOfIndices);
			assert(nullptr == mVertexArray.getPointer());
			assert(mSubMeshes.empty());
			assert(isInvalid(mSkeletonResourceId));

			// Call base implementation
			IResource::initializeElement(meshResourceId);
		}

		inline void deinitializeElement()
		{
			// Reset everything
			setInvalid(mMinimumBoundingBoxPosition.x);
			setInvalid(mMinimumBoundingBoxPosition.y);
			setInvalid(mMinimumBoundingBoxPosition.z);
			setInvalid(mMaximumBoundingBoxPosition.x);
			setInvalid(mMaximumBoundingBoxPosition.y);
			setInvalid(mMaximumBoundingBoxPosition.z);
			setInvalid(mBoundingSpherePosition.x);
			setInvalid(mBoundingSpherePosition.y);
			setInvalid(mBoundingSpherePosition.z);
			setInvalid(mBoundingSphereRadius);
			mNumberOfVertices = 0;
			mNumberOfIndices = 0;
			mVertexArray = nullptr;
			mSubMeshes.clear();
			setInvalid(mSkeletonResourceId);

			// Call base implementation
			IResource::deinitializeElement();
		}


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Bounding, the bounding sphere radius is enclosing the bounding box (don't use the inner bounding box radius)
		glm::vec3 mMinimumBoundingBoxPosition;
		glm::vec3 mMaximumBoundingBoxPosition;
		glm::vec3 mBoundingSpherePosition;
		float	  mBoundingSphereRadius;
		// Vertex and index data
		uint32_t				  mNumberOfVertices;	///< Number of vertices
		uint32_t				  mNumberOfIndices;		///< Number of indices
		Renderer::IVertexArrayPtr mVertexArray;			///< Vertex array object (VAO), can be a null pointer
		// Sub-meshes
		SubMeshes				  mSubMeshes;			///< Sub-meshes
		// Optional skeleton
		SkeletonResourceId		  mSkeletonResourceId;	///< Resource ID of the used skeleton, can be invalid


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
