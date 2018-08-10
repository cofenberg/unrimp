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
#include "RendererRuntime/Public/Resource/Material/MaterialProperties.h"
#include "RendererRuntime/Public/Resource/CompositorNode/Pass/ICompositorResourcePass.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace RendererRuntime
{


	//[-------------------------------------------------------]
	//[ Global definitions                                    ]
	//[-------------------------------------------------------]
	typedef StringId AssetId;				///< Asset identifier, internally just a POD "uint32_t", string ID scheme is "<project name>/<asset type>/<asset category>/<asset name>"
	typedef StringId MaterialTechniqueId;	///< Material technique identifier, internally just a POD "uint32_t", result of hashing the material technique name


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Compositor resource pass compute
	*
	*  @remarks
	*    A compositor resource pass compute instance is using a material resource for compute rendering. This material resource
	*    can be defined by providing an material asset ID. Since compositor material blueprints are usually highly
	*    specialized for a certain task, it would be annoying to have to define a material asset for each and every
	*    compositor material. So, it's also supported to define a material blueprint asset directly.
	*/
	class CompositorResourcePassCompute : public ICompositorResourcePass
	{


	//[-------------------------------------------------------]
	//[ Friends                                               ]
	//[-------------------------------------------------------]
		friend class CompositorPassFactory;	// The only one allowed to create instances of this class


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static constexpr uint32_t TYPE_ID = STRING_ID("Compute");


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		CompositorResourcePassCompute(const CompositorTarget& compositorTarget, AssetId materialBlueprintAssetId, const MaterialProperties& materialProperties);

		inline virtual ~CompositorResourcePassCompute() override
		{
			// Nothing here
		}

		inline bool isMaterialDefinitionMandatory() const
		{
			return mMaterialDefinitionMandatory;
		}

		inline AssetId getMaterialAssetId() const
		{
			return mMaterialAssetId;
		}

		inline MaterialTechniqueId getMaterialTechniqueId() const
		{
			return mMaterialTechniqueId;
		}

		inline AssetId getMaterialBlueprintAssetId() const
		{
			return mMaterialBlueprintAssetId;
		}

		inline const MaterialProperties& getMaterialProperties() const
		{
			return mMaterialProperties;
		}


	//[-------------------------------------------------------]
	//[ Public virtual RendererRuntime::ICompositorResourcePass methods ]
	//[-------------------------------------------------------]
	public:
		inline virtual CompositorPassTypeId getTypeId() const override
		{
			return TYPE_ID;
		}

		virtual void deserialize(uint32_t numberOfBytes, const uint8_t* data) override;


	//[-------------------------------------------------------]
	//[ Protected methods                                     ]
	//[-------------------------------------------------------]
	protected:
		inline explicit CompositorResourcePassCompute(const CompositorTarget& compositorTarget) :
			ICompositorResourcePass(compositorTarget),
			mMaterialDefinitionMandatory(true)
		{
			// Nothing here
		}

		inline CompositorResourcePassCompute(const CompositorTarget& compositorTarget, bool materialDefinitionMandatory) :
			ICompositorResourcePass(compositorTarget),
			mMaterialDefinitionMandatory(materialDefinitionMandatory)
		{
			// Nothing here
		}

		explicit CompositorResourcePassCompute(const CompositorResourcePassCompute&) = delete;
		CompositorResourcePassCompute& operator=(const CompositorResourcePassCompute&) = delete;


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		bool				mMaterialDefinitionMandatory;
		AssetId				mMaterialAssetId;			///< If material blueprint asset ID is set, material asset ID must be invalid
		MaterialTechniqueId	mMaterialTechniqueId;		///< Must always be valid
		AssetId				mMaterialBlueprintAssetId;	///< If material asset ID is set, material blueprint asset ID must be invalid
		MaterialProperties	mMaterialProperties;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // RendererRuntime
