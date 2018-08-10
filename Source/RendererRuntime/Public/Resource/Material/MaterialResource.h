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
#include "RendererRuntime/Public/Resource/Material/MaterialProperties.h"


//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace RendererRuntime
{
	class Renderable;
	class MaterialTechnique;
	class MaterialResourceLoader;
	template <class ELEMENT_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class PackedElementManager;
	template <class TYPE, class LOADER_TYPE, typename ID_TYPE, uint32_t MAXIMUM_NUMBER_OF_ELEMENTS> class ResourceManagerTemplate;
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId MaterialTechniqueId;	///< Material technique identifier, internally just a POD "uint32_t", result of hashing the material technique name
	typedef uint32_t MaterialResourceId;	///< POD material resource identifier


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Material resource
	*/
	class MaterialResource final : public IResource
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class Renderable;																			// Must be able to attach/detach itself from the material resource
		friend class MaterialResourceLoader;
		friend class MaterialResourceManager;
		friend class MaterialBlueprintResourceManager;
		friend ResourceManagerTemplate<MaterialResource, MaterialResourceLoader, MaterialResourceId, 4096>;	// Type definition of template class
		friend PackedElementManager<MaterialResource, MaterialResourceId, 4096>;							// Type definition of template class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		typedef std::vector<MaterialTechnique*> SortedMaterialTechniqueVector;

		// Fixed build in material properties
		static constexpr uint32_t RENDER_QUEUE_INDEX_PROPERTY_ID = STRING_ID("RenderQueueIndex");	///< "RenderQueueIndex", value type = "INTEGER", usage = "STATIC", value range = [0, 255]
		static constexpr uint32_t CAST_SHADOWS_PROPERTY_ID		 = STRING_ID("CastShadows");		///< "CastShadows", value type = "BOOLEAN", usage = "STATIC"


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Return the parent material resource ID
		*
		*  @return
		*    The parent material resource ID, invalid if there's no parent
		*/
		inline MaterialResourceId getParentMaterialResourceId() const
		{
			return mParentMaterialResourceId;
		}

		/**
		*  @brief
		*    Set the parent material resource ID
		*
		*  @param[in] parentMaterialResourceId
		*    Parent material resource ID, can be invalid
		*
		*  @note
		*    - Parent material resource must be fully loaded
		*    - All property values will be reset
		*/
		RENDERERRUNTIME_API_EXPORT void setParentMaterialResourceId(MaterialResourceId parentMaterialResourceId);

		/**
		*  @brief
		*    Return the sorted material technique vector
		*
		*  @return
		*    The sorted material technique vector
		*/
		inline const SortedMaterialTechniqueVector& getSortedMaterialTechniqueVector() const
		{
			return mSortedMaterialTechniqueVector;
		}

		/**
		*  @brief
		*    Return a material technique by its ID
		*
		*  @param[in] materialTechniqueId
		*    ID of the material technique to return
		*
		*  @return
		*    The requested material technique, null pointer on error, don't destroy the returned instance
		*/
		RENDERERRUNTIME_API_EXPORT MaterialTechnique* getMaterialTechniqueById(MaterialTechniqueId materialTechniqueId) const;

		/**
		*  @brief
		*    Destroy all material techniques
		*/
		RENDERERRUNTIME_API_EXPORT void destroyAllMaterialTechniques();

		//[-------------------------------------------------------]
		//[ Property                                              ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Return the material properties
		*
		*  @return
		*    The material properties
		*/
		inline const MaterialProperties& getMaterialProperties() const
		{
			return mMaterialProperties;
		}

		/**
		*  @brief
		*    Return the material properties as sorted vector
		*
		*  @return
		*    The material properties as sorted vector
		*/
		inline const MaterialProperties::SortedPropertyVector& getSortedPropertyVector() const
		{
			return mMaterialProperties.getSortedPropertyVector();
		}

		/**
		*  @brief
		*    Remove all material properties
		*/
		inline void removeAllProperties()
		{
			return mMaterialProperties.removeAllProperties();
		}

		/**
		*  @brief
		*    Return a material property by its ID
		*
		*  @param[in] materialPropertyId
		*    ID of the material property to return
		*
		*  @return
		*    The requested material property, null pointer on error, don't destroy the returned instance
		*/
		inline const MaterialProperty* getPropertyById(MaterialPropertyId materialPropertyId) const
		{
			return mMaterialProperties.getPropertyById(materialPropertyId);
		}

		/**
		*  @brief
		*    Set a material property value by its ID
		*
		*  @param[in] materialPropertyId
		*    ID of the material property to set the value from
		*  @param[in] materialPropertyValue
		*    The material property value to set
		*  @param[in] materialPropertyUsage
		*    The material property usage
		*
		*  @return
		*    "true" if a material property change has been detected, else "false"
		*/
		inline bool setPropertyById(MaterialPropertyId materialPropertyId, const MaterialPropertyValue& materialPropertyValue, MaterialProperty::Usage materialPropertyUsage = MaterialProperty::Usage::UNKNOWN)
		{
			return setPropertyByIdInternal(materialPropertyId, materialPropertyValue, materialPropertyUsage, true);
		}

		//[-------------------------------------------------------]
		//[ Internal                                              ]
		//[-------------------------------------------------------]
		// TODO(co)
		void releaseTextures();


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		inline MaterialResource() :
			mParentMaterialResourceId(getInvalid<MaterialResourceId>())
		{
			// Nothing here
		}

		virtual ~MaterialResource() override;
		explicit MaterialResource(const MaterialResource&) = delete;
		MaterialResource& operator=(const MaterialResource&) = delete;
		RENDERERRUNTIME_API_EXPORT MaterialResource& operator=(MaterialResource&& materialResource);

		//[-------------------------------------------------------]
		//[ "RendererRuntime::PackedElementManager" management    ]
		//[-------------------------------------------------------]
		inline void initializeElement(MaterialResourceId materialResourceId)
		{
			// Sanity checks
			assert(isInvalid(mParentMaterialResourceId));
			assert(mSortedChildMaterialResourceIds.empty());
			assert(mSortedMaterialTechniqueVector.empty());
			assert(mMaterialProperties.getSortedPropertyVector().empty());

			// Call base implementation
			IResource::initializeElement(materialResourceId);
		}

		void deinitializeElement();

		/**
		*  @brief
		*    Set a material property value by its ID
		*
		*  @param[in] materialPropertyId
		*    ID of the material property to set the value from
		*  @param[in] materialPropertyValue
		*    The material property value to set
		*  @param[in] materialPropertyUsage
		*    The material property usage
		*  @param[in] changeOverwrittenState
		*    Change overwritten state?
		*
		*  @return
		*    Pointer to the added or changed property, null pointer if no material property change has been detected, don't destroy the returned instance
		*/
		RENDERERRUNTIME_API_EXPORT bool setPropertyByIdInternal(MaterialPropertyId materialPropertyId, const MaterialPropertyValue& materialPropertyValue, MaterialProperty::Usage materialPropertyUsage, bool changeOverwrittenState);


	//[-------------------------------------------------------]
	//[ Private definitions                                   ]
	//[-------------------------------------------------------]
	private:
		typedef std::vector<MaterialResourceId> SortedChildMaterialResourceIds;
		typedef std::vector<Renderable*>		AttachedRenderables;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		MaterialResourceId			   mParentMaterialResourceId;
		SortedChildMaterialResourceIds mSortedChildMaterialResourceIds;
		SortedMaterialTechniqueVector  mSortedMaterialTechniqueVector;
		MaterialProperties			   mMaterialProperties;
		AttachedRenderables			   mAttachedRenderables;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
